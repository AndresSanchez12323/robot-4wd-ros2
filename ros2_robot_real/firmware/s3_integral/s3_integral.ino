#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <atomic>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "EnlaceRobot.h"
#include "LidarD6.h"
#include "EstadoIntegral.h"
#include "Navegacion.h"
#include "SupervisorMarcha.h"
#include "GuardiaROS.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona ESP32S3 Dev Module."
#endif

Navegacion navegacion;
GuardiaROS guardiaROS;
std::atomic<uint32_t> geometria{0}; // RAM: vuelve a confirmar el frente tras reiniciar.
uint32_t corrida = 0, bloqueadaDevkit = 0;
ConfigNavegacion configuracion() {
  const uint32_t g = geometria.load();
  return {int(g & 511), int((g >> 9) & 2047), bool(g & 0x80000000u), !(g & 0x40000000u)};
}
HardwareSerial enlace(1);
HardwareSerial puertoLidar(2);
ReceptorRobot receptor;
LidarD6 lidar;
httpd_handle_t servidorWeb = nullptr, servidorVideo = nullptr;
QueueHandle_t ordenesROS = nullptr;
portMUX_TYPE muxEstado = portMUX_INITIALIZER_UNLOCKED;
EstadoIntegral estado;
std::atomic<bool> solicitudStop{false};
std::atomic<bool> videoActivo{false};
std::atomic<uint32_t> ultimoFrame{0};
std::atomic<uint32_t> fpsDecimas{0};
bool camaraIniciada = false, servidoresListos = false;
bool devkitIMU = false, devkitMoviendo = false, devkitRecibido = false;
float distanciaDevkit = 0;
uint32_t sesion = 0, secuencia = 0, orden = 0;
uint32_t ultimoDevkit = 0, ultimoLatido = 0, ultimoLog = 0;
uint32_t ultimoArranqueLidar = 0;
char mensajeDevkit[300] = "Esperando DevKitC";
char ultimaParada[96] = "Ninguna";
char motivoDevkit[48] = "Sin telemetria";
char evento[96] = "Abre la pagina y espera los indicadores";
char lineaUSB[32]{};
size_t longitudUSB = 0;
bool descartarUSB = false;

const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot 4WD · navegación continua</title>
<style>
body{font:16px Arial;background:#142b43;color:#fff;margin:20px auto;max-width:1100px;padding:0 16px}
h1{font-size:25px}.paneles{display:flex;gap:16px;flex-wrap:wrap}.panel{flex:1;min-width:280px}
img,canvas{width:100%;max-width:520px;background:#203c55;border-radius:12px}
button{font-size:19px;padding:14px 22px;border:0;border-radius:8px;margin:8px 10px 8px 0;cursor:pointer}
#avanzar{background:#77dec4}#parar{background:#ff6666;font-weight:bold}button:disabled{opacity:.4}
pre{white-space:pre-wrap;background:#203c55;padding:14px;border-radius:8px;overflow-wrap:anywhere}
</style></head><body>
<h1>Robot 4WD · navegación continua</h1>
<p>ROS2-1: la marcha se inicia desde ROS 2. Esta pagina conserva diagnostico y STOP.</p>
<p>ARRANCAR inicia el avance sin límite de centímetros. Ante un obstáculo elige izquierda o derecha según el espacio observado. Si falta espacio, espera y reanuda al recuperarlo. PARAR / STOP cancela la marcha. La espera por LiDAR o espacio se recupera sola. STOP, fallos de enlace, MPU o rueda cancelan la marcha. El video puede reconectarse sin cancelar una marcha con LiDAR sano.</p>
<div class="paneles"><div class="panel"><h2>Camara OV5640</h2>
<img id="video" alt="Video de la camara"><br>
<button onclick="reconectar()">Reconectar video</button>
<a style="color:#8deadb" href="/captura" target="_blank">Captura</a></div>
<div class="panel"><h2>LiDAR · vista de 72 sectores</h2>
<canvas id="radar" width="420" height="420"></canvas>
<p>0 grados es la referencia del LiDAR. El dibujo no supone el frente del carro. Escala 3 m.</p></div></div>
<p>Configuración antes de arrancar: coloca un objeto delante del carro y pulsa su punto en el radar para elegir el ángulo frontal. Retira el objeto antes de arrancar.</p>
<label>Frente del carro (grados): <input id="frente" type="number" min="0" max="359" step="1" placeholder="Selecciona en radar"></label>
<label>Radio hasta la esquina (mm): <input id="radio" type="number" min="100" max="1000" value="150"></label>
<label>Sentido angular del LiDAR: <select id="sentido"><option value="">Selecciona tras comprobar</option><option value="1">Crece hacia la derecha física</option><option value="-1">Crece hacia la izquierda física</option></select></label>
<p>Con el carro detenido, mueve el objeto del frente a la derecha: confirma si el ángulo aumenta o disminuye. Este ajuste orienta la elección de lado.</p>
<button onclick="configurar()">Confirmar frente y radio</button><p id="configActual">Frente sin confirmar. Radio indicado por ti: 150 mm.</p>
<button id="avanzar" disabled onclick="mandar('avanzar')">ARRANCAR</button>
<button id="parar" onclick="mandar('stop')">PARAR / STOP</button>
<pre id="estado">Esperando datos...</pre><pre id="diagnostico"></pre><pre id="devkit"></pre>
<p>Mantener esta pagina abierta. Al perder el enlace con la pagina se cancela el movimiento.</p>
<script>
const boton=document.getElementById('avanzar');
const radar=document.getElementById('radar');const ctx=radar.getContext('2d');
radar.addEventListener('click',e=>{const b=radar.getBoundingClientRect();const x=(e.clientX-b.left)/b.width*420-210,y=(e.clientY-b.top)/b.height*420-210;
 if(Math.hypot(x,y)<8)return;document.getElementById('frente').value=Math.round((Math.atan2(x,-y)*180/Math.PI+360)%360)%360});
async function configurar(){
 const f=document.getElementById('frente'),r=document.getElementById('radio');
 const sentido=document.getElementById('sentido').value;
 if(sentido!=='1'&&sentido!=='-1'){alert('Confirma hacia que lado aumentan los angulos del LiDAR.');return}
 if(!f.value||!r.value||!f.checkValidity()||!r.checkValidity()){alert('Indica el ángulo frontal y el radio real del chasis.');return}
 try{const res=await fetch('/configurar?frente='+f.value+'&radio='+r.value+'&confirmar=1&sentido='+document.getElementById('sentido').value,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('configActual').textContent=await res.text()}catch(e){}
}
function reconectar(){document.getElementById('video').src='http://'+location.hostname+':81/stream?t='+Date.now()}
async function mandar(accion){
 boton.disabled=true;
 try{const r=await fetch('/'+accion,{method:'POST',headers:{'X-Robot':'1'}});
 document.getElementById('estado').textContent=await r.text()}catch(e){}
}
function dibujar(mapa,frente,configurada){
 ctx.clearRect(0,0,420,420);ctx.strokeStyle='#56738b';ctx.fillStyle='#8deadb';
 for(let r=42;r<=168;r+=42){ctx.beginPath();ctx.arc(210,210,r,0,2*Math.PI);ctx.stroke()}
 ctx.beginPath();ctx.moveTo(210,20);ctx.lineTo(210,400);ctx.moveTo(20,210);ctx.lineTo(400,210);ctx.stroke();
 ctx.font='14px Arial';ctx.fillText('0 grados',220,20);
 mapa.forEach((d,i)=>{if(d>0&&d<=3000){const a=i*5*Math.PI/180;
 const r=d/3000*180;ctx.beginPath();ctx.arc(210+Math.sin(a)*r,210-Math.cos(a)*r,3,0,2*Math.PI);ctx.fill()}});
 ctx.fillStyle='#ffcc55';ctx.fillRect(205,205,10,10);
 if(configurada){const a=frente*Math.PI/180;ctx.strokeStyle='#ffcc55';ctx.beginPath();ctx.moveTo(210,210);ctx.lineTo(210+Math.sin(a)*110,210-Math.cos(a)*110);ctx.stroke()}
}
async function actualizar(){
 try{
  const r=await fetch('/estado',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
  const si=v=>v?'OK':'PENDIENTE';
  document.getElementById('estado').textContent=
   'Camara: '+si(d.camara)+' | FPS enviados: '+d.fps+'\n'+
   'LiDAR: '+si(d.lidar)+' | sectores: '+d.sectores+'/72 | minimo: '+d.minimo+' mm\n'+
   'UART: '+si(d.enlace)+' | MPU: '+si(d.imu)+' | operador: '+si(d.operador)+'\n'+
   'Recorrido de ruedas (incluye giros): '+d.distancia+' cm | marcha autónoma: '+(d.activa?'ACTIVA':'PARADA')+' | motores: '+(d.moviendo?'SI':'NO')+'\n'+
   'LiDAR paquetes: '+d.paquetes+' | errores: '+d.errores+' | barridos: '+d.barridos+'\n'+d.evento+'\nUltima parada: '+d.ultimaParada;
  document.getElementById('devkit').textContent='DevKitC\n'+d.devkit;
  document.getElementById('diagnostico').textContent=
   'DIAGNOSTICO D6 | bytes: '+d.bytesLidar+' | cabeceras invalidas: '+d.erroresCabecera+
   ' | checksum invalidos: '+d.erroresChecksum+' | inicios validos: '+d.iniciosLidar+'\n'+(d.lidar?'Barridos válidos recibidos':d.diagnosticoLidar);
  boton.disabled=true;
  document.getElementById('configActual').textContent=d.configurada?'Frente confirmado: '+d.frente+' grados | radio: '+d.radio+' mm | angulos crecen hacia '+(d.horario?'derecha':'izquierda'):'Confirma el frente del carro para habilitar la marcha';
  dibujar(d.mapa,d.frente,d.configurada);
 }catch(e){boton.disabled=true;document.getElementById('estado').textContent='Sin datos actuales del S3. Comprueba RobotCam; los valores anteriores pueden estar desactualizados.';
 document.getElementById('devkit').textContent='DevKitC: sin telemetria actual; esperando reconexion';
 document.getElementById('diagnostico').textContent='Sin diagnostico actual; esperando reconexion';}
 setTimeout(actualizar,300);
}
reconectar();actualizar();
</script></body></html>
)HTML";

bool peticionLocal(httpd_req_t *req) {
  char valor[4];
  return httpd_req_get_hdr_value_str(req, "X-Robot", valor, sizeof(valor)) == ESP_OK &&
         strcmp(valor, "1") == 0;
}

esp_err_t paginaWeb(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

esp_err_t estadoWeb(httpd_req_t *req) {
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado);
  copia = estado;
  portEXIT_CRITICAL(&muxEstado);
  char mapa[512];
  size_t usados = 0;
  for (size_t i = 0; i < 72; ++i) {
    usados += snprintf(mapa + usados, sizeof(mapa) - usados, "%s%u", i ? "," : "", copia.mapa[i]);
  }
  char json[2800];
  snprintf(json, sizeof(json),
    "{\"version\":\"ROS2-1\",\"sesion\":%lu,\"orden\":%lu,\"corrida\":%lu,\"muestraDevkit\":%lu,\"fase\":%u,\"giro\":%d,\"espacioIzq\":%u,\"espacioDer\":%u,"
    "\"horario\":%u,\"ultimaParada\":\"%s\",\"activa\":%u,\"configurada\":%u,\"frente\":%d,\"radio\":%d,\"permiso\":%u,\"camara\":%u,\"lidar\":%u,\"enlace\":%u,\"imu\":%u,"
    "\"puedeArmar\":%u,\"moviendo\":%u,\"operador\":%u,\"minimo\":%u,\"sectores\":%u,\"paquetes\":%lu,"
    "\"errores\":%lu,\"barridos\":%lu,\"fps\":%.1f,\"distancia\":%.1f,"
    "\"bytesLidar\":%lu,\"erroresCabecera\":%lu,\"erroresChecksum\":%lu,\"iniciosLidar\":%lu,"
    "\"diagnosticoLidar\":\"%s\","
    "\"devkit\":\"%s\",\"evento\":\"%s\",\"mapa\":[%s]}",
    (unsigned long)copia.sesionRos, (unsigned long)copia.ultimaOrdenRos,
    (unsigned long)copia.corrida, (unsigned long)copia.muestraDevkit,
    copia.fase, copia.giro, copia.espacioIzq, copia.espacioDer,
    copia.horario, copia.ultimaParada, copia.activa, copia.configurada, copia.frente, copia.radio, copia.permiso, copia.camara, copia.lidar, copia.enlace, copia.imu, copia.puedeArmar, copia.moviendo,
    copia.operador, copia.minimo, copia.sectores, (unsigned long)copia.paquetes,
    (unsigned long)copia.errores, (unsigned long)copia.barridos, copia.fps, copia.distancia,
    (unsigned long)copia.bytesLidar, (unsigned long)copia.erroresCabecera,
    (unsigned long)copia.erroresChecksum, (unsigned long)copia.iniciosLidar, copia.diagnosticoLidar,
    copia.devkit, copia.evento, mapa);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

esp_err_t avanzarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Inicia la marcha desde ROS 2");
}

bool leerEnteroROS(const char *query, const char *nombre, uint32_t &resultado) {
  char valor[11];
  if (httpd_query_key_value(query, nombre, valor, sizeof(valor)) != ESP_OK || !valor[0]) return false;
  resultado = 0;
  for (const char *p = valor; *p; ++p) {
    if (*p < '0' || *p > '9' || resultado > (UINT32_MAX - (*p - '0')) / 10) return false;
    resultado = resultado * 10 + (*p - '0');
  }
  return true;
}

esp_err_t ordenWebROS(httpd_req_t *req) {
  char query[110];
  OrdenROS orden{};
  uint32_t inicio, preferencia;
  if (!peticionLocal(req) ||
      httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      !leerEnteroROS(query, "sesion", orden.sesion) || !leerEnteroROS(query, "id", orden.id) ||
      !leerEnteroROS(query, "inicio", inicio) || inicio > 1 ||
      !leerEnteroROS(query, "preferencia", preferencia) || preferencia > 2 ||
      !orden.id || orden.sesion != sesion) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Orden ROS invalida");
  }
  orden.inicio = inicio;
  orden.preferencia = int(preferencia) - 1;
  orden.recibida = millis();
  if (xQueueSend(ordenesROS, &orden, 0) != pdPASS)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Cola ROS ocupada");
  httpd_resp_set_status(req, "202 Accepted");
  return httpd_resp_send(req, "Orden en cola; confirmar con telemetria", HTTPD_RESP_USE_STRLEN);
}

esp_err_t configurarWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  EstadoIntegral copia;
  portENTER_CRITICAL(&muxEstado); copia = estado; portEXIT_CRITICAL(&muxEstado);
  if (copia.activa || copia.moviendo) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pulsa STOP antes de configurar");
  char query[120], f[12], r[12], c[4], sentido[4];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "frente", f, sizeof(f)) != ESP_OK ||
      httpd_query_key_value(query, "radio", r, sizeof(r)) != ESP_OK ||
      httpd_query_key_value(query, "confirmar", c, sizeof(c)) != ESP_OK || strcmp(c, "1") != 0 ||
      httpd_query_key_value(query, "sentido", sentido, sizeof(sentido)) != ESP_OK ||
      (strcmp(sentido, "1") != 0 && strcmp(sentido, "-1") != 0))
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Faltan frente, radio y confirmacion");
  char *finF, *finR;
  long frente = strtol(f, &finF, 10), radio = strtol(r, &finR, 10);
  if (!*f || !*r || *finF || *finR || frente < 0 || frente >= 360 || radio < 100 || radio > 1000)
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Geometria fuera de rango");
  geometria.store(0x80000000u | (strcmp(sentido, "-1") == 0 ? 0x40000000u : 0u) | (uint32_t(radio) << 9) | uint32_t(frente));
  solicitudStop.store(true); // evita carreras con un ARRANCAR simultaneo.
  return httpd_resp_send(req, "Frente y radio confirmados. Retira el objeto de referencia.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t pararWeb(httpd_req_t *req) {
  if (!peticionLocal(req)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Peticion invalida");
  solicitudStop.store(true);
  return httpd_resp_send(req, "STOP solicitado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t capturaWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  camera_fb_t *imagen = esp_camera_fb_get();
  if (!imagen) return httpd_resp_send_500(req);
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t resultado = httpd_resp_send(req, (const char *)imagen->buf, imagen->len);
  esp_camera_fb_return(imagen);
  return resultado;
}

esp_err_t videoWeb(httpd_req_t *req) {
  if (!camaraIniciada) return httpd_resp_send_500(req);
  esp_err_t resultado = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=robotframe");
  if (resultado != ESP_OK) return resultado;
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  videoActivo.store(true);
  uint32_t cuadros = 0, inicioFPS = millis();
  while (true) {
    camera_fb_t *imagen = esp_camera_fb_get();
    if (!imagen) { resultado = ESP_FAIL; break; }
    char cabecera[100];
    const int n = snprintf(cabecera, sizeof(cabecera),
      "\r\n--robotframe\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)imagen->len);
    resultado = httpd_resp_send_chunk(req, cabecera, n);
    if (resultado == ESP_OK) resultado = httpd_resp_send_chunk(req, (const char *)imagen->buf, imagen->len);
    esp_camera_fb_return(imagen);
    if (resultado != ESP_OK) break;
    ultimoFrame.store(millis());
    ++cuadros;
    const uint32_t transcurrido = millis() - inicioFPS;
    if (transcurrido >= 3000) {
      fpsDecimas.store(cuadros * 10000UL / transcurrido);
      inicioFPS = millis();
      cuadros = 0;
    }
    delay(1);
  }
  videoActivo.store(false);
  fpsDecimas.store(0);
  return resultado;
}

bool registrarRuta(httpd_handle_t servidor, const char *uri, httpd_method_t metodo,
                   esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t ruta = {};
  ruta.uri = uri;
  ruta.method = metodo;
  ruta.handler = handler;
  return httpd_register_uri_handler(servidor, &ruta) == ESP_OK;
}

bool iniciarServidores() {
  httpd_config_t web = HTTPD_DEFAULT_CONFIG();
  web.server_port = 80;
  web.max_uri_handlers = 9;
  web.stack_size = 8192;
  web.send_wait_timeout = 2;
  if (httpd_start(&servidorWeb, &web) != ESP_OK) return false;
  bool bien = registrarRuta(servidorWeb, "/", HTTP_GET, paginaWeb) &&
    registrarRuta(servidorWeb, "/estado", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/telemetria", HTTP_GET, estadoWeb) &&
    registrarRuta(servidorWeb, "/ros/orden", HTTP_POST, ordenWebROS) &&
    registrarRuta(servidorWeb, "/avanzar", HTTP_POST, avanzarWeb) &&
    registrarRuta(servidorWeb, "/configurar", HTTP_POST, configurarWeb) &&
    registrarRuta(servidorWeb, "/stop", HTTP_POST, pararWeb) &&
    registrarRuta(servidorWeb, "/captura", HTTP_GET, capturaWeb);
  httpd_config_t video = HTTPD_DEFAULT_CONFIG();
  video.server_port = 81;
  video.ctrl_port = web.ctrl_port + 1;
  video.stack_size = 8192;
  video.send_wait_timeout = 2;
  bien = bien && httpd_start(&servidorVideo, &video) == ESP_OK &&
         registrarRuta(servidorVideo, "/stream", HTTP_GET, videoWeb);
  if (!bien) {
    if (servidorVideo) httpd_stop(servidorVideo);
    httpd_stop(servidorWeb);
  }
  return bien;
}

bool iniciarCamara() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11; config.pin_d1 = 9; config.pin_d2 = 8; config.pin_d3 = 10;
  config.pin_d4 = 12; config.pin_d5 = 18; config.pin_d6 = 17; config.pin_d7 = 16;
  config.pin_xclk = 15; config.pin_pclk = 13; config.pin_vsync = 6; config.pin_href = 7;
  config.pin_sccb_sda = 4; config.pin_sccb_scl = 5;
  config.pin_pwdn = -1; config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 16;
  const bool psram = psramFound();
  config.fb_count = psram ? 2 : 1;
  config.fb_location = psram ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode = psram ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  Serial.printf("PSRAM=%s | buferes=%d\n", psram ? "SI" : "NO", config.fb_count);
  const esp_err_t resultado = esp_camera_init(&config);
  Serial.printf("CAMARA init=0x%X\n", (unsigned)resultado);
  return resultado == ESP_OK;
}

void procesarDevkit(const char *texto) {
  unsigned long recibida, activa, bloqueada;
  unsigned int moviendo, imu;
  float distancia; char causa[48];
  if (sscanf(texto, "A1 %lu %lu %lu %u %u %f %47s", &recibida, &activa, &bloqueada,
      &moviendo, &imu, &distancia, causa) != 7 || recibida != sesion ||
      moviendo > 1 || imu > 1 || !isfinite(distancia) || distancia < 0) return;
  for (const char *p = texto; *p; ++p) {
    if (*p == '"' || *p == '\\' || static_cast<unsigned char>(*p) < 32) return;
  }
  devkitRecibido = true; ultimoDevkit = millis(); devkitMoviendo = moviendo;
  devkitIMU = imu; distanciaDevkit = distancia; bloqueadaDevkit = bloqueada;
  snprintf(motivoDevkit, sizeof(motivoDevkit), "%s", causa);
  snprintf(mensajeDevkit, sizeof(mensajeDevkit), "%s", texto);
}

void leerUSB() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineaUSB[longitudUSB] = '\0';
      if (!descartarUSB) {
        if (strcmp(lineaUSB, "STOP") == 0) solicitudStop.store(true);
        else Serial.println("Comando USB: STOP. Para arrancar usa ROS 2.");
      }
      longitudUSB = 0;
      descartarUSB = false;
    } else if (longitudUSB < sizeof(lineaUSB) - 1) lineaUSB[longitudUSB++] = c;
    else descartarUSB = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  sesion = esp_random();
  if (!sesion) sesion = 1;
  ordenesROS = xQueueCreate(1, sizeof(OrdenROS));
  if (!ordenesROS) { Serial.println("ERROR memoria para cola de ordenes"); return; }
  enlace.setRxBufferSize(2048);
  enlace.begin(115200, SERIAL_8N1, 42, 41);
  enviarRobot(enlace, "STOP");
  puertoLidar.setRxBufferSize(8192);
  puertoLidar.begin(230400, SERIAL_8N1, 1, 2);
  camaraIniciada = iniciarCamara();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("RobotCam", "CAMBIA_CLAVE_ROBOTCAM", 1, false, 4)) {
    Serial.println("ERROR WIFI");
    return;
  }
  ArduinoOTA.setHostname("robot-s3");
  ArduinoOTA.setPassword("CAMBIA_CLAVE_ROBOTCAM");
  ArduinoOTA.onStart([]() {
    guardiaROS.parar();
    solicitudStop.store(true);
    enviarRobot(enlace, "STOP");
  });
  ArduinoOTA.begin();
  servidoresListos = iniciarServidores();
  Serial.printf("S3 ROS2-1 | camara=%u web=%u | LiDAR RX1 TX2\n", camaraIniciada, servidoresListos);
  Serial.print("Pagina http://"); Serial.println(WiFi.softAPIP());
  Serial.println("Wi-Fi RobotCam | clave CAMBIA_CLAVE_ROBOTCAM | motores parados hasta nueva orden");
  const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
  puertoLidar.write(iniciar, sizeof(iniciar));
  ultimoArranqueLidar = millis();
}

void loop() {
  if (!ordenesROS) { delay(100); return; }
  if (!navegacion.activa()) ArduinoOTA.handle();
  for (size_t i = 0; i < 4096 && puertoLidar.available(); ++i) {
    lidar.recibir(puertoLidar.read(), millis());
  }
  for (size_t i = 0; i < 1024 && enlace.available(); ++i) {
    const char *texto = receptor.recibir(enlace.read());
    if (texto) procesarDevkit(texto);
  }
  leerUSB();
  const uint32_t ahora = millis();
  if (!lidar.vigente(ahora) && ahora - ultimoArranqueLidar > 4000) {
    const uint8_t iniciar[] = {0xAA, 0x55, 0xF0, 0x0F};
    puertoLidar.write(iniciar, sizeof(iniciar));
    ultimoArranqueLidar = ahora;
  }
  // Leer primero los sellos escritos por las tareas HTTP y despues el reloj;
  // evita una resta unsigned negativa si llega un frame durante esta iteracion.
  const uint32_t frame = ultimoFrame.load();
  const uint32_t ahoraSalud = millis();
  const bool video = camaraIniciada && videoActivo.load() && frame && ahoraSalud - frame < 1000;
  const bool operador = guardiaROS.vigente(ahoraSalud);
  const bool sensor = lidar.vigente(ahora);
  const bool devkit = devkitRecibido && ahora - ultimoDevkit <= 600;
  const ConfigNavegacion config = configuracion();
  const bool salud = servidoresListos && devkit && devkitIMU;
  const char *obstaculo = config.valida() ? navegacion.evaluar(lidar.distanciasAvance, config) : "Confirma frente y radio del chasis";
  const char *giro = config.valida() ? navegacion.evaluar(lidar.distancias, config, true) : obstaculo;
  const bool puedeArmar = salud && config.valida();
  const bool permiso = puedeArmar && sensor && (!obstaculo || !giro);
  const bool estabaActiva = navegacion.activa();
  if (solicitudStop.exchange(false)) {
    xQueueReset(ordenesROS);
    guardiaROS.parar();
    navegacion.parar(); snprintf(ultimaParada, sizeof(ultimaParada), "STOP o cambio de configuracion"); corrida = 0; enviarRobot(enlace, "STOP");
  } else {
    const char *causa = causaParada(servidoresListos, operador, devkit, devkitIMU,
                                    corrida && bloqueadaDevkit == corrida, motivoDevkit);
    if (navegacion.activa() && causa) {
      snprintf(ultimaParada, sizeof(ultimaParada), "%s", causa);
      navegacion.fallar(ultimaParada);
      xQueueReset(ordenesROS);
      guardiaROS.parar();
    }
    OrdenROS solicitada;
    if (!causa || !estabaActiva) {
      if (xQueueReceive(ordenesROS, &solicitada, 0) == pdPASS &&
          guardiaROS.aceptar(solicitada, sesion, millis(), navegacion.activa(), puedeArmar) &&
          solicitada.inicio) {
        if (++orden == 0) ++orden;
        corrida = orden; navegacion.arrancar(config, ahora);
        if (!permiso) navegacion.esperarArranque(lidar.barridos);
      }
    }
    navegacion.preferenciaROS = guardiaROS.preferencia;
    navegacion.actualizar(lidar.distancias, config, sensor, lidar.barridos, ahora, lidar.distanciasAvance);
    if (!navegacion.activa()) corrida = 0;
    if (estabaActiva && !navegacion.activa()) enviarRobot(enlace, "STOP");
  }
  if (navegacion.fase == Navegacion::ESPERANDO_ESPACIO || navegacion.fase == Navegacion::ESPERANDO_DATOS)
    snprintf(evento, sizeof(evento), "Espera automatica: %.70s", navegacion.motivo);
  else if (navegacion.activa()) snprintf(evento, sizeof(evento), "%s", navegacion.motivo);
  else if (navegacion.fase == Navegacion::FALLO) snprintf(evento, sizeof(evento), "%s. Pulsa ARRANCAR para reintentar.", navegacion.motivo);
  else snprintf(evento, sizeof(evento), "%s", !config.valida() ? "Confirma frente y radio del chasis" :
      !sensor ? "Esperando barridos validos del LiDAR" : !devkit ? "Esperando DevKit con firmware autonomo V1" :
      !devkitIMU ? "MPU sin datos" : obstaculo && giro ? obstaculo :
      !operador ? "Esperando orden de ROS 2" : navegacion.motivo);
  if (ahora - ultimoLatido >= 100) {
    ultimoLatido = ahora;
    char mensaje[140];
    snprintf(mensaje, sizeof(mensaje), "V1 %lu %lu %lu %d %d %d %d",
      (unsigned long)sesion, (unsigned long)++secuencia, (unsigned long)corrida,
      navegacion.izquierda, navegacion.izquierda, navegacion.derecha, navegacion.derecha);
    enviarRobot(enlace, mensaje);
  }
  const uint16_t espacioIzq = sensor && config.valida() ? navegacion.espacioLado(lidar.distancias, config, 1) : 0;
  const uint16_t espacioDer = sensor && config.valida() ? navegacion.espacioLado(lidar.distancias, config, -1) : 0;
  portENTER_CRITICAL(&muxEstado);
  estado.sesionRos = sesion; estado.ultimaOrdenRos = guardiaROS.ultimaOrden;
  estado.corrida = corrida; estado.muestraDevkit = ultimoDevkit;
  estado.fase = navegacion.fase; estado.giro = navegacion.sentido;
  estado.espacioIzq = espacioIzq; estado.espacioDer = espacioDer;
  estado.activa = navegacion.activa();
  estado.horario = config.horario;
  memcpy(estado.ultimaParada, ultimaParada, sizeof(estado.ultimaParada));
  estado.configurada = config.valida(); estado.frente = config.frente; estado.radio = config.radio;
  estado.permiso = permiso;
  estado.puedeArmar = puedeArmar;
  estado.camara = video;
  estado.lidar = sensor;
  estado.enlace = devkit;
  estado.imu = devkit && devkitIMU;
  estado.moviendo = devkit && devkitMoviendo;
  estado.operador = operador;
  estado.minimo = lidar.minimo;
  estado.sectores = lidar.cobertura;
  estado.paquetes = lidar.paquetes;
  estado.bytesLidar = lidar.bytesRecibidos;
  estado.erroresCabecera = lidar.erroresCabecera;
  estado.erroresChecksum = lidar.erroresChecksum;
  estado.iniciosLidar = lidar.inicios;
  memcpy(estado.diagnosticoLidar, lidar.diagnostico, sizeof(estado.diagnosticoLidar));
  estado.errores = lidar.errores;
  estado.barridos = lidar.barridos;
  estado.fps = fpsDecimas.load() / 10.0f;
  estado.distancia = distanciaDevkit;
  memcpy(estado.mapa, lidar.distancias, sizeof(estado.mapa));
  memcpy(estado.devkit, mensajeDevkit, sizeof(estado.devkit));
  memcpy(estado.evento, evento, sizeof(estado.evento));
  portEXIT_CRITICAL(&muxEstado);
  if (ahora - ultimoLog >= 2000) {
    ultimoLog = ahora;
    Serial.printf("ESTADO | cam=%u lidar=%u uart=%u imu=%u permiso=%u | min=%u mm | sectores=%u/72 | paquetes=%lu errores=%lu\n",
      video, sensor, devkit, devkitIMU, permiso, lidar.minimo, lidar.cobertura,
      (unsigned long)lidar.paquetes, (unsigned long)lidar.errores);
  }
  delay(1);
}
