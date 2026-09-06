"""Author: Angelis Pseftis. Derived vector state boards, not runtime animations."""
from pathlib import Path
import json,html,math,textwrap
P=Path(__file__).resolve().parent

def render(item):
 out=['<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="1150" viewBox="0 0 1600 1150"><metadata>Author: Angelis Pseftis. Derived motion reference; not Unreal evidence.</metadata><rect width="1600" height="1150" fill="#eeeae1"/>']
 def text(x,y,s,size=19,color='#26333c'):
  out.append(f'<text x="{x}" y="{y}" font-family="sans-serif" font-size="{size}" fill="{color}">{html.escape(s)}</text>')
 def line(x,y,a,b,color='#545961',w=3):out.append(f'<path d="M{x},{y} L{a},{b}" fill="none" stroke="{color}" stroke-width="{w}"/>')
 def circle(x,y,r,color='#725480',fill='none') :out.append(f'<circle cx="{x}" cy="{y}" r="{r}" stroke="{color}" stroke-width="3" fill="{fill}"/>')
 def box(x,y,w,h,color='#545961',fill='none'):out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" stroke="{color}" stroke-width="2" fill="{fill}"/>')
 text(40,52,item['title']+' — state reference',32);text(40,88,'SCHEMATIC • CANDIDATE • NOT AN ANIMATION CLIP OR RUNTIME TEST',18)
 title=item['title']
 for n,phase in enumerate(item['phases']):
  x=40+(n%3)*520;y=125+(n//3)*295;box(x,y,500,275,'#b7b4aa');text(x+18,y+32,phase['label'],24)
  cx=x+250;cy=y+135
  if 'Concordance' in title:
   for i in range(6):
    a=math.radians(45+i*54)
    for j in range(2):
     registered=(n in [2] and i==0) or n>=4; r=55+(0 if registered else j*10)
     px=cx+r*math.cos(a);py=cy+r*math.sin(a);tone='#a3a0a0' if n==5 or (n in [1,2] and i==0 and j==1) else '#795389'
     line(px-15*math.sin(a),py+15*math.cos(a),px+15*math.sin(a),py-15*math.cos(a),tone)
   text(x+18,y+216,'12 identities; overlap is not deletion',16)
  elif 'shadow' in title:
   line(cx-80,cy+30,cx,cy-65);line(cx,cy-65,cx+80,cy+30);line(cx-28,cy+45,cx+6,cy-80);line(cx+6,cy-80,cx+28,cy+45)
   if n in [1,3,4,5]:line(cx-80,cy+35,cx+105,cy+55,'#8c959e',10)
   if n in [2,3,4,5]:line(cx-75,cy+40,cx+65,cy-3,'#a887b4',10)
   text(x+18,y+216,'Gray A / violet B are effect vectors',16)
  elif 'upkeep' in title:
   box(cx-65,cy-60,130,115,'#6c5778');color='#be448d' if n in [1,2,3] else '#88729b'
   circle(cx,cy,65,color);text(cx-12,cy+3,str(n) if n in [1,2,3] else ('OFF' if n==4 else 'OK'),26)
   if n==4:line(cx-60,cy-45,cx+60,cy+35,'#a5a0a5')
   text(x+18,y+216,'State event; no free-running payment',16)
  elif 'Phase Anchor' in title:
   box(cx-15,cy-65,30,115,'#795389')
   if n not in [3,4]:circle(cx,cy,65,'#ad4688')
   box(cx+115,cy,30,35,'#ad4688' if n==4 else '#545961')
   text(x+18,y+216,'Ring size illustrative; membership external',16)
  elif 'matrix' in title:
   out.append(f'<ellipse cx="{cx}" cy="{cy}" rx="115" ry="55" fill="{("#796338" if n in [1,2,3] else "#454641")}" stroke="#56554e" stroke-width="4"/>')
   for i in range(3):
    yy=cy-20+i*20;out.append(f'<path d="M{cx-85},{yy} Q{cx},{yy-18+(n%3)*8} {cx+85},{yy}" fill="none" stroke="#c1a564" stroke-width="2"/>')
   text(x+18,y+216,'Unbroken fluid surface; no rigid crust',16)
  else:
   line(cx-120,cy+30,cx+120,cy+30)
   for i in range(5):
    xx=cx-96+i*48;active=n in [1,2,3] and (i==(n-1) or n==3)
    if 'Spine' in title:circle(xx,cy+10-abs(i-2)*5,12,'#b2883e' if active else '#646157','#b2883e' if active else 'none')
    else:line(xx,cy+30,xx+12,cy-45+(i%2)*12,'#b2883e' if active else '#646157',7)
   text(x+18,y+216,'Abstract groups; anatomy count not specified',16)
  for k,s in enumerate(textwrap.wrap(phase['caption'],49)):text(x+18,y+240+k*20,s,17)
 y=745
 for label,key in [('REDUCED MOTION','reduced_motion'),('CANCEL / RESTORE','interruption_and_restore'),('LATER ACCEPTANCE','acceptance_evidence_required')]:
  text(40,y,label,17);y+=24
  for s in textwrap.wrap(item[key],150):text(40,y,s,18);y+=23
  y+=12
 for s in textwrap.wrap('Contracts: '+', '.join(item['contracts']),150):text(40,y,s,15);y+=20
 text(40,1115,'Author: Angelis Pseftis • State board only; labels specify intent, not executed simulation evidence.',16)
 out.append('</svg>');return '\n'.join(out)
if __name__=='__main__':
 target=P/'storyboards';target.mkdir(exist_ok=True)
 for item in json.loads((P/'motion-packages.json').read_text())['items']:(target/(item['gap_id']+'.svg')).write_text(render(item))
 print('7 source-bound vector state boards generated')
