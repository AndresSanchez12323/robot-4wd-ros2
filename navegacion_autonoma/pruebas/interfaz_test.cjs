const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const source=fs.readFileSync(path.join(__dirname,'../s3_integral/s3_integral.ino'),'utf8');
const js=source.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements={},events={},requests=[];
const ctx=new Proxy({},{get:()=>()=>{}});
for(const id of ['avanzar','radar','video','frente','radio','estado','configActual','diagnostico','devkit','sentido']) {
 elements[id]={disabled:false,textContent:'',value:'',checkValidity:()=>true,
  getContext:()=>ctx,addEventListener:(type,fn)=>events[type]=fn,
  getBoundingClientRect:()=>({left:0,top:0,width:420,height:420})};
}
elements.sentido.value="1";
let state={permiso:1,activa:0,configurada:1,frente:0,radio:150,camara:1,lidar:1,enlace:1,imu:1,
 operador:1,moviendo:0,fps:10,distancia:0,sectores:68,minimo:900,paquetes:12,errores:0,barridos:1,
 bytesLidar:1000,erroresCabecera:0,erroresChecksum:0,iniciosLidar:2,mapa:Array(72).fill(900),devkit:'A1',evento:'Listo'};
let disconnected=false;
const sandbox={document:{getElementById:id=>elements[id]},location:{hostname:'192.168.4.1'},
 setTimeout:()=>{},alert:()=>{},fetch:async(url,options)=>{
  requests.push({url,options});if(disconnected)throw Error('Offline');
  return {ok:true,json:async()=>state,text:async()=> 'OK'};
 }};
vm.createContext(sandbox);vm.runInContext(js,sandbox);
(async()=>{
 await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.camara=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,false);
 state.camara=1; // video no decide permiso, pero la pagina informa su estado.
 state.activa=1;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 await sandbox.mandar('stop');assert.equal(requests.at(-1).url,'/stop');
 state.activa=0;state.permiso=0;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 elements.frente.value='0';elements.radio.value='150';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1&sentido=1');
 elements.sentido.value='-1';await sandbox.configurar();
 assert.equal(requests.at(-1).url,'/configurar?frente=0&radio=150&confirmar=1&sentido=-1');
 const numeroAntes=requests.length;elements.sentido.value='';await sandbox.configurar();
 assert.equal(requests.length,numeroAntes); // no confirmar un sentido supuesto.
 events.click({clientX:310,clientY:210});assert.equal(elements.frente.value,90);
 disconnected=true;await sandbox.actualizar();assert.equal(elements.avanzar.disabled,true);
 assert.match(elements.devkit.textContent,/sin telemetria actual/);
 console.log('OK: interfaz real, bloqueo de ARRANCAR, STOP, frente cero, seleccion de angulo y desconexion');
})().catch(e=>{console.error(e);process.exitCode=1});
