import * as THREE from 'three';
import {GLTFLoader} from 'three/examples/jsm/loaders/GLTFLoader.js';
import {OrbitControls} from 'three/examples/jsm/controls/OrbitControls.js';
const $=id=>document.getElementById(id), status=t=>$('status').textContent=t;
let renderer;try{renderer=new THREE.WebGLRenderer({antialias:true,alpha:false});}catch(e){status('WebGL nem elérhető ezen a készüléken');throw e;}
renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.setClearColor(0x152632);$('view').appendChild(renderer.domElement);
const scene=new THREE.Scene(),camera=new THREE.OrthographicCamera(-5,5,5,-5,.01,100000);camera.position.set(10,10,10);const controls=new OrbitControls(camera,renderer.domElement);controls.enableDamping=true;controls.minPolarAngle=controls.maxPolarAngle=Math.acos(1/Math.sqrt(3));controls.target.set(0,0,0);controls.update();
const ambient=new THREE.HemisphereLight(0xddeeff,0x57483b,2.1);scene.add(ambient);const sun=new THREE.DirectionalLight(0xffe8cd,2.5);sun.position.set(8,15,5);scene.add(sun);const grid=new THREE.GridHelper(100,100,0x587286,0x344652);scene.add(grid);
let records=[], selected=-1, busy=false;let hour=12,windPower=.5;const clock=new THREE.Clock();const timeUniform={value:0},powerUniform={value:.5};const loader=new GLTFLoader(), keys=['x','y','z','rotationY','scale'];
function resize(){const w=$('view').clientWidth,h=$('view').clientHeight;if(!w||!h)return;renderer.setSize(w,h,false);const a=w/h;camera.left=-5*a;camera.right=5*a;camera.top=5;camera.bottom=-5;camera.updateProjectionMatrix();}new ResizeObserver(resize).observe($('view'));
function update(){const dt=Math.min(clock.getDelta(),.05);timeUniform.value+=dt;for(const r of records){if(r.mixer)r.mixer.update(dt);if(r.windUniform)r.windUniform.value=r.wind||0;}controls.update();renderer.render(scene,camera);requestAnimationFrame(update);}resize();update();
function list(){const s=$('objects');s.replaceChildren(...records.map((r,i)=>{const o=document.createElement('option');o.value=i;o.textContent=(i+1)+' · '+r.file;return o}));s.selectedIndex=selected;for(const k of keys)$(k).value=selected<0?'':records[selected][k];motionUI();}
function transform(r){r.group.position.set(r.x,r.y,r.z);r.group.rotation.y=THREE.MathUtils.degToRad(r.rotationY);r.group.scale.setScalar(r.scale);}
async function add(p){if(records.length>=64)throw Error('Legfeljebb 64 objektum');const gltf=await loader.loadAsync('/model?name='+encodeURIComponent(p.file));const r={file:p.file,x:Number(p.x||0),y:Number(p.y||0),z:Number(p.z||0),rotationY:Number(p.rotationY||0),scale:Number(p.scale||1),group:new THREE.Group(),clips:gltf.animations||[],wind:Number(p.wind||0),clip:String(p.clip||''),playing:p.playing===true,speed:Number(p.speed||1)};r.group.add(gltf.scene);r.mixer=new THREE.AnimationMixer(gltf.scene);installWind(r,gltf.scene);transform(r);scene.add(r.group);records.push(r);chooseClip(r,r.clip,r.playing);selected=records.length-1;list();}

function installWind(r,model){
  r.windUniform={value:r.wind||0};
  model.traverse(o=>{
    if(!o.isMesh||o.isSkinnedMesh||!o.geometry||!o.material)return;
    o.geometry.computeBoundingBox();const b=o.geometry.boundingBox;if(!b||b.isEmpty())return;
    const low=b.min.y,span=Math.max(.0001,b.max.y-low);
    const patch=mat=>{const m=mat.clone();m.onBeforeCompile=shader=>{
      shader.uniforms.taksTime=timeUniform;shader.uniforms.taksPower=powerUniform;shader.uniforms.taksWind=r.windUniform;
      shader.uniforms.taksLow={value:low};shader.uniforms.taksSpan={value:span};
      shader.vertexShader='uniform float taksTime,taksPower,taksWind,taksLow,taksSpan;\n'+shader.vertexShader;
      shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>','#include <begin_vertex>\nfloat h=clamp((position.y-taksLow)/taksSpan,0.0,1.0); float bend=taksWind*taksPower*h*h*taksSpan; transformed.x+=bend*(0.065*sin(taksTime*2.0+position.y*0.8)+0.025*sin(taksTime*5.3+position.z*1.5)); transformed.z+=bend*0.035*sin(taksTime*1.7+position.y);');
    };m.customProgramCacheKey=()=> 'taks-wind-v1';return m;};
    o.material=Array.isArray(o.material)?o.material.map(patch):patch(o.material);
  });
}
function chooseClip(r,name,playing){
  if(r.action){r.action.stop();r.action=null;}
  const clip=r.clips.find(c=>c.name===name);r.clip=clip?clip.name:'';r.playing=!!clip&&playing;
  if(clip){r.action=r.mixer.clipAction(clip);r.action.reset().play();r.action.paused=!r.playing;r.action.timeScale=r.speed;}
}
function motionUI(){const r=records[selected];const el=$('clip');el.replaceChildren();const empty=document.createElement('option');empty.value='';empty.textContent='Nincs animáció kiválasztva';el.append(empty);if(r)for(const c of r.clips){const o=document.createElement('option');o.value=c.name;o.textContent=c.name||'Névtelen klip';el.append(o);}el.value=r?.clip||'';$('play').disabled=!r?.action;$('play').textContent=r?.playing?'Szünet':'Lejátszás';$('wind').value=r?.wind||0;$('wind').disabled=!r;$('speed').value=r?.speed||1;$('speed').disabled=!r;}
$('clip').onchange=()=>{const r=records[selected];if(r){chooseClip(r,$('clip').value,true);motionUI();}};
$('play').onclick=()=>{const r=records[selected];if(r?.action){r.playing=!r.playing;r.action.paused=!r.playing;motionUI();}};
$('speed').oninput=()=>{const r=records[selected];if(r){r.speed=Number($('speed').value);if(r.action)r.action.timeScale=r.speed;}};
$('wind').oninput=()=>{const r=records[selected];if(r)r.wind=Number($('wind').value);};
function lighting(){const a=(hour-6)/24*Math.PI*2, height=Math.sin(a);sun.position.set(Math.cos(a)*15,Math.max(0.1,height*20),5);sun.intensity=Math.max(0,height)*2.5;ambient.intensity=.2+Math.max(0,height)*1.9;renderer.setClearColor(new THREE.Color(0x0a1221).lerp(new THREE.Color(0x364b5b),Math.max(0,height)));powerUniform.value=windPower;}
$('hour').oninput=()=>{hour=Number($('hour').value);lighting();};$('windPower').oninput=()=>{windPower=Number($('windPower').value);lighting();};lighting();

function fit(){if(!records.length)return;const b=new THREE.Box3();for(const r of records)b.expandByObject(r.group);if(b.isEmpty())return;const c=b.getCenter(new THREE.Vector3()), size=b.getSize(new THREE.Vector3()).length();controls.target.copy(c);camera.position.copy(c).add(new THREE.Vector3(1,1,1).normalize().multiplyScalar(Math.max(10,size*2)));camera.zoom=10/Math.max(1,size*1.25);camera.updateProjectionMatrix();controls.update();}
$('objects').onchange=()=>{selected=Number($('objects').value);list()};
$('apply').onclick=()=>{if(selected<0)return;const n={};for(const k of keys){n[k]=Number($(k).value);if(!Number.isFinite(n[k])||Math.abs(n[k])>100000||(k==='scale'&&n[k]<=0)){status('Érvénytelen érték');return;}}Object.assign(records[selected],n);transform(records[selected]);status('Módosítva – a Mentés gombbal rögzíthető.');};
$('add').onclick=async()=>{if(busy||!$('library').value)return;busy=true;try{status('GLB betöltése…');await add({file:$('library').value,x:records.length*2,scale:1});fit();status('Modell hozzáadva.');}catch(e){status('GLB hiba: '+e.message);}finally{busy=false;}};
$('delete').onclick=()=>{if(selected<0)return;const r=records.splice(selected,1)[0];r.mixer?.stopAllAction();r.mixer?.uncacheRoot(r.group.children[0]);scene.remove(r.group);r.group.traverse(o=>{o.geometry?.dispose();if(o.material){for(const m of [].concat(o.material)){for(const v of Object.values(m))if(v?.isTexture)v.dispose();m.dispose();}}});selected=records.length?Math.min(selected,records.length-1):-1;list();};
$('fit').onclick=fit;$('save').onclick=()=>{if(busy)return;const data=records.map(r=>({file:r.file,x:r.x,y:r.y,z:r.z,rotationY:r.rotationY,scale:r.scale,wind:r.wind,clip:r.clip,playing:r.playing,speed:r.speed}));const answer=window.TAKS?.save(JSON.stringify({placements:data,environment:{hour,windPower}}));status(answer==='OK'?'Jelenet elmentve.':answer||'Mentési hiba.');};
(async()=>{busy=true;try{const p=await fetch('/project').then(r=>r.json());$('library').replaceChildren(...p.library.map(n=>{const o=document.createElement('option');o.value=n;o.textContent=n;return o}));hour=Number(p.environment?.hour??12);windPower=Number(p.environment?.windPower??.5);$('hour').value=hour;$('windPower').value=windPower;lighting();let failed=0;for(const r of (p.placements||[]).slice(0,64))try{await add(r)}catch(e){failed++;}fit();status(failed?failed+' modell nem tölthető be; a mentés felülírja a jelenetet.':p.library.length?'Húzás: forgatás • két ujj: nagyítás':'Előbb importálj GLB-t a főmenüben.');if(failed)$('save').disabled=true;}catch(e){status('Projekt hiba: '+e.message);}finally{busy=false;}})();