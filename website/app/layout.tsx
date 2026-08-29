import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'Echoes of the Broken Sun | A science-fantasy RTS in development',
  description: 'Enter Soryn, a broken world where every future has a cost. Follow the development of Echoes of the Broken Sun.',
  openGraph: { title: 'Echoes of the Broken Sun', description: 'Every future has a cost.', images: ['/hero-soryn.png'] },
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return <html lang="en"><body>{children}</body></html>;
}
