const test = require("node:test");
const assert = require("node:assert/strict");
const { crearResolvers, tokenValido, robotListo } = require("./resolvers");

const env = {
  TOKEN: "secreto",
  EXIGIR_GPU: "1",
  LLM_URL: "http://llm",
  VLM_URL: "http://vlm",
  VLA_URL: "http://vla",
  GPU_URL: "http://gpu",
  ROBOT_URL: "http://robot",
  ORQUESTADOR_URL: "http://orquestador",
};

function json(ok, datos, status) {
  return {
    ok, status: status || (ok ? 200 : 503),
    text: async () => JSON.stringify(datos),
  };
}

test("el token distinto no pasa", () => {
  assert.equal(tokenValido("secreto", "secreto"), true);
  assert.equal(tokenValido("otro", "secreto"), false);
  assert.equal(tokenValido("", "secreto"), false);
});

test("sin geometria el carro no esta listo", () => {
  assert.equal(robotListo({ version: "1.6", enlace: 1, imu: 1, lidar: 1, configurada: 0, permiso: 0 }), false);
  assert.equal(robotListo({ version: "1.6", enlace: 1, imu: 1, lidar: 1, configurada: 1, permiso: 1, operador: 1 }), true);
});

test("si falta un servicio la mision no se pide", async () => {
  const pedidos = [];
  const fetcher = async (url, opciones) => {
    pedidos.push(url + " " + (opciones.method || "GET"));
    if (url.endsWith("/salud")) {
      const sano = !url.startsWith("http://vlm");
      return json(sano, { sano, detalle: sano ? "listo" : "apagado" });
    }
    return json(true, { version: "1.6", enlace: 1, imu: 1, lidar: 1, configurada: 1, permiso: 1, operador: 1 });
  };
  const resolvers = crearResolvers(env, fetcher);
  const salud = await resolvers.Query.salud();
  assert.equal(salud.listo, false);
  assert.equal(salud.servicios.find((s) => s.nombre === "vlm").sano, false);
  await assert.rejects(
    () => resolvers.Mutation.iniciar(null, { texto: "ve a la gaseosa" }, { authorization: "Bearer secreto" }),
    /Faltan servicios: vlm/,
  );
  assert.equal(pedidos.some((p) => p.includes("/api/mision")), false);
});

test("con todos listos la orden sale hacia el orquestador", async () => {
  let cuerpo = "";
  const fetcher = async (url, opciones) => {
    if (url.endsWith("/api/mision") && opciones.method === "POST") {
      cuerpo = opciones.body;
      assert.equal(opciones.headers["X-Entrada"], "graphql");
      return json(true, { sano: true }, 202);
    }
    if (url.endsWith("/api/mision")) {
      return json(true, { estado: "en_marcha", detalle: "Buscando gaseosa", lugares: [5], nombres: ["gaseosa"], lugar: "gaseosa", accion: "explorar" });
    }
    if (url.endsWith("/estado")) {
      return json(true, { version: "1.6", enlace: 1, imu: 1, lidar: 1, configurada: 1, permiso: 1, operador: 1 });
    }
    return json(true, { sano: true, detalle: "listo" });
  };
  const resolvers = crearResolvers(env, fetcher);
  const mision = await resolvers.Mutation.iniciar(null, { texto: "ve a la gaseosa" }, { authorization: "Bearer secreto" });
  assert.equal(cuerpo, JSON.stringify({ texto: "ve a la gaseosa" }));
  assert.equal(mision.nombres[0], "gaseosa");
  assert.deepEqual(mision.bitacora, []);
  await assert.rejects(
    () => resolvers.Mutation.iniciar(null, { texto: "ve a la gaseosa" }, { authorization: "Bearer otro" }),
    /token/,
  );
});
