// Author: Angelis Pseftis. Source-level UI and drawing checks, not browser rendering.
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const h=fs.readFileSync(__dirname+'/motion-review.html','utf8'),s=h.match(/<script>([\s\S]*)<\/script>/)[1];
let draws=0;const ctx=new Proxy({}, {get:(o,k)=>o[k]??((...a)=>{for(const v of a)if(typeof v==='number')assert(Number.isFinite(v));draws++}),set:(o,k,v)=>(o[k]=v,true)});
function el(){return {value:0,checked:false,textContent:'',children:[],append(x){this.children.push(x)},replaceChildren(){this.children=[]},setAttribute(k,v){this[k]=v},getContext(){return ctx}}}
const nodes={};const doc={querySelector:k=>nodes[k]??=(el()),querySelectorAll:()=>nodes['#tabs'].children,createElement:el};
const box={document:doc,matchMedia:()=>({matches:false}),requestAnimationFrame:()=>{},console};vm.createContext(box);vm.runInContext(s,box);
vm.runInContext(`for(let m=0;m<studies.length;m++){select(m);for(let v of [0,.2,.4,.6,.8,1]){t=v;draw();reduce.checked=true;draw();reduce.checked=false}}`,box);
assert.equal(nodes['#tabs'].children.length,11);assert.equal(nodes['#asset'].children.length,21);
vm.runInContext(`for(let i=0;i<data.packages.length;i++){asset.value=i;renderRecord()}select(0);document.querySelector('#play').onclick();if(!playing)throw Error('play');document.querySelector('#play').onclick();if(playing)throw Error('pause');slider.value=1000;slider.oninput();if(t!==1)throw Error('scrub');document.querySelector('#reset').onclick();if(t!==0)throw Error('reset');`,box);
console.log(JSON.stringify({studies:11,samples:132,candidate_records:21,control_checks:4,draw_calls:draws,evidence:'source execution with mocked canvas; NOT browser visual verification'}));
