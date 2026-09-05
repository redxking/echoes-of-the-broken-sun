// Author and owner: Angelis Pseftis
// Decode retained review movies; outputs metadata and representative PNG frames.
import Foundation
import AVFoundation
import AppKit
import CoreImage

@main struct InspectReviewMovie {
    static func main() async {
        do { try await inspect() }
        catch {
            FileHandle.standardError.write(Data("[ECHOES_MOVIE_INSPECTION_FAILED] \(error)\n".utf8))
            exit(1)
        }
    }

    static func inspect() async throws {
        guard CommandLine.arguments.count == 3 || CommandLine.arguments.count == 4 else {
            throw NSError(domain: "EchoesMovieInspection", code: 1,
                userInfo: [NSLocalizedDescriptionKey: "Usage: inspect_review_movie movie.mov output-directory [sample-seconds,comma-separated]"])
        }
        let source = URL(fileURLWithPath: CommandLine.arguments[1])
        let output = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
        try FileManager.default.createDirectory(at: output, withIntermediateDirectories: true)
        let asset = AVURLAsset(url: source)
        let duration = try await asset.load(.duration).seconds
        let tracks = try await asset.loadTracks(withMediaType: .video)
        guard let track = tracks.first else {
            throw NSError(domain: "EchoesMovieInspection", code: 2,
                userInfo: [NSLocalizedDescriptionKey: "Movie has no video track"])
        }
        let size = try await track.load(.naturalSize)
        let fps = try await track.load(.nominalFrameRate)
        print("[ECHOES_MOVIE_METADATA] duration=\(duration) size=\(size) fps=\(fps)")
        let reader = try AVAssetReader(asset: asset)
        let stream = AVAssetReaderTrackOutput(track: track, outputSettings:
            [kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA])
        reader.add(stream)
        guard reader.startReading() else { throw reader.error! }
        var samples = 0
        var firstPTS: Double? = nil
        var lastPTS = 0.0
        var frames: [[String: Any]] = []
        let targets: [Double]
        if CommandLine.arguments.count == 4 {
            let values = CommandLine.arguments[3].split(separator: ",").compactMap { Double($0) }
            guard !values.isEmpty, values.count <= 120,
                  values.allSatisfy({ $0.isFinite && $0 >= 0 && $0 < duration }),
                  zip(values, values.dropFirst()).allSatisfy({ $0 < $1 }) else {
                throw NSError(domain: "EchoesMovieInspection", code: 5,
                    userInfo: [NSLocalizedDescriptionKey: "Sample times must be increasing, within the movie, and at most120."])
            }
            targets = values
        } else {
            targets = [min(0.25, duration / 4), duration / 2, max(0, duration - 0.2)]
        }
        let context = CIContext(options: [.useSoftwareRenderer: true])
        while let sample = stream.copyNextSampleBuffer() {
            let time = CMSampleBufferGetPresentationTimeStamp(sample).seconds
            if firstPTS == nil { firstPTS = time }
            lastPTS = time
            samples += 1
            if frames.count < targets.count && time >= targets[frames.count],
               let buffer = CMSampleBufferGetImageBuffer(sample) {
                let ciImage = CIImage(cvPixelBuffer: buffer)
                guard let cgImage = context.createCGImage(ciImage, from: ciImage.extent),
                      let png = NSBitmapImageRep(cgImage: cgImage).representation(using: .png, properties: [:]) else {
                    throw NSError(domain: "EchoesMovieInspection", code: 4)
                }
                let path = output.appendingPathComponent("frame-\(frames.count).png")
                try png.write(to: path)
                frames.append(["requested_seconds": targets[frames.count], "actual_seconds": time, "path": path.path])
            }
        }
        guard reader.status == .completed else {
            throw reader.error ?? NSError(domain: "EchoesMovieInspection", code: 3)
        }
        print("[ECHOES_MOVIE_SAMPLES] count=\(samples) first=\(firstPTS ?? -1) last=\(lastPTS)")
        let audioTracks = try await asset.loadTracks(withMediaType: .audio)
        let metadata: [String: Any] = ["author": "Angelis Pseftis", "source": source.path,
            "utc": ISO8601DateFormatter().string(from: Date()), "duration_seconds": duration,
            "width": size.width, "height": size.height, "nominal_fps": fps,
            "video_samples": samples, "first_pts": firstPTS ?? -1, "last_pts": lastPTS,
            "audio_tracks": audioTracks.count, "frames": frames,
            "evidence_class": "Decoded movie metadata and sampled frames; motion acceptance separate"]
        let json = try JSONSerialization.data(withJSONObject: metadata, options: [.prettyPrinted, .sortedKeys])
        try json.write(to: output.appendingPathComponent("movie-inspection.json"))
        print(String(decoding: json, as: UTF8.self))
    }
}
