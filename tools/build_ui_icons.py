"""Shared 16x16 game UI icons. SVG source and PNG both use this pixel grid."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]/'assets/icons/actions'
ROOT.mkdir(parents=True,exist_ok=True)
PALETTE={'K':'#233549','W':'#fff9dd','R':'#f25755','D':'#b72b43','Y':'#ffd15c','O':'#ae6d2b','B':'#65cbef','C':'#2882bc','G':'#65ce8e','H':'#26815f'}
def blank(): return [['.' for x in range(16)] for y in range(16)]
def rect(p,x,y,w,h,c):
 for j in range(y,y+h):
  for i in range(x,x+w):
   if 0<=i<16 and 0<=j<16:p[j][i]=c

def ball():
 p=blank()
 for y,line in enumerate(['................','.....KKKKKK.....','...KKRRRRRRKK...','..KRRWWRRRRRRK..','..KRWWRRRRRRRK..','.KRRRRRRRRRRRRK.','.KRRRRKKKKRRRRK.','.KKKKKKWWKKKKKK.','.KWWWWKWWKWWWWK.','.KWWWWKKKKWWWWK.','.KWWWWWWWWWWWWK.','..KWWWWWWWWWWK..','..KWWWWWWWWWWK..','...KKWWWWWWKK...','.....KKKKKK.....','................']):p[y]=list(line)
 return p
icons={}
p=ball();icons['start']=p
p=ball();rect(p,5,5,2,6,'K');rect(p,9,5,2,6,'K');icons['pause']=p
p=ball();rect(p,10,0,5,2,'K');rect(p,13,1,2,5,'K');rect(p,10,1,4,1,'Y');rect(p,13,2,1,3,'Y');rect(p,11,4,3,1,'Y');icons['restart']=p
p=blank();rect(p,3,1,10,14,'K');rect(p,2,3,12,10,'K');rect(p,4,2,8,12,'R');rect(p,3,4,10,8,'R');rect(p,5,3,6,5,'K');rect(p,6,4,4,3,'B');rect(p,5,10,6,3,'K');rect(p,6,10,4,1,'W');rect(p,7,11,2,1,'W');icons['save']=p
p=blank();rect(p,5,1,6,3,'K');rect(p,6,2,4,1,'W');rect(p,3,4,10,11,'K');rect(p,2,6,12,7,'K');rect(p,4,5,8,9,'Y');rect(p,3,7,10,5,'Y');rect(p,4,5,8,4,'O');rect(p,5,5,6,2,'Y');rect(p,7,7,2,3,'K');rect(p,7,8,2,1,'W');rect(p,5,11,6,2,'O');icons['load']=p
p=blank();rect(p,1,6,4,5,'K');rect(p,2,7,2,3,'B');rect(p,11,6,4,5,'K');rect(p,12,7,2,3,'B');rect(p,5,3,6,11,'K');rect(p,4,5,8,7,'K');rect(p,6,4,4,9,'C');rect(p,5,6,6,5,'B');rect(p,6,5,2,2,'W');rect(p,7,7,3,4,'B');icons['upgrade']=p
p=blank();rect(p,5,1,6,14,'K');rect(p,3,3,10,10,'K');rect(p,2,5,12,6,'K');rect(p,5,2,6,12,'O');rect(p,4,4,8,8,'Y');rect(p,3,6,10,4,'Y');rect(p,6,4,4,8,'O');rect(p,5,6,6,4,'O');rect(p,7,5,2,6,'Y');rect(p,6,7,4,2,'Y');rect(p,5,3,2,2,'W');icons['sell']=p
# Strategy-menu symbols follow the same 16px palette and hard outlines.
p=blank();rect(p,3,3,10,10,'K');rect(p,4,4,8,8,'W');rect(p,5,5,6,6,'R');rect(p,6,6,4,4,'W');rect(p,7,7,2,2,'R');rect(p,7,0,2,4,'K');rect(p,7,12,2,4,'K');rect(p,0,7,4,2,'K');rect(p,12,7,4,2,'K');icons['strategy_nearest']=p
p=blank()
for y,line in enumerate(['................','................','...KKK....KKK...','..KRRRK..KRRRK..','.KRWWRRKKRRRRRK.','.KRWRRRRRRRRRRK.','.KRRRRRRRRRRRRK.','..KRRRRRRRRRRK..','...KRRRRRRRRK...','....KRRRRRRK....','.....KRRRRK.....','......KRRK......','.......KK.......','................','................','................']):p[y]=list(line)
icons['strategy_lowesthp']=p
p=ball()
for y,row in enumerate(p):
 for x,c in enumerate(row):
  if c=='R':p[y][x]='B'
rect(p,6,4,4,7,'W');rect(p,4,6,8,3,'W');rect(p,7,5,2,5,'R');rect(p,5,6,6,2,'R');icons['strategy_closesttocore']=p
p=blank();rect(p,2,4,5,1,'B');rect(p,1,7,5,1,'B');rect(p,2,10,4,1,'B');rect(p,8,1,5,3,'K');rect(p,7,4,5,4,'K');rect(p,6,7,7,3,'K');rect(p,8,9,3,3,'K');rect(p,7,12,3,2,'K');rect(p,9,2,3,2,'Y');rect(p,8,4,3,4,'Y');rect(p,7,8,5,1,'Y');rect(p,9,9,1,3,'Y');rect(p,8,12,1,1,'Y');icons['strategy_fastest']=p
p=blank()
for y in range(5):rect(p,3+y,5+y,10-y*2,1,'K')
for y in range(3):rect(p,5+y,6+y,6-y*2,1,'R')
icons['menu_down']=p
for name,pixels in icons.items():
 im=Image.new('RGBA',(16,16))
 for y,row in enumerate(pixels):
  for x,c in enumerate(row):
   if c!='.':im.putpixel((x,y),tuple(bytes.fromhex(PALETTE[c][1:]))+(255,))
 im.resize((32,32),Image.Resampling.NEAREST).save(ROOT/(name+'.png'))
 parts=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" shape-rendering="crispEdges">']
 for y,row in enumerate(pixels):
  for x,c in enumerate(row):
   if c!='.':parts.append(f'<rect x="{x}" y="{y}" width="1" height="1" fill="{PALETTE[c]}"/>')
 parts.append('</svg>')
 (ROOT/(name+'.svg')).write_text(''.join(parts),encoding='utf-8')
print('Generated %d matching pixel icons (SVG + PNG)' % len(icons))
