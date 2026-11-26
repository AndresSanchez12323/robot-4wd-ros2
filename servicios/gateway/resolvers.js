/** El gateway no decide la mision: consulta a cada servicio y, si todos responden, pide el recorrido. */

const crypto = require("crypto");

const typeDefs = `#graphql
  type Servicio {
    nombre: String!
    sano: Boolean!
    detalle: String!
  }

  type Salud {
    listo: Boolean!
    servicios: [Servicio!]!
  }

  type Mision {
    estado: String!
    detalle: String!
    lugares: [Int!]!
    nombres: [String!]!
    lugar: String!
    accion: String!
  }

  type Query {
    salud: Salud!
    mision: Mision!
  }

  type Mutation {
    iniciar(texto: String!): Mision!
    parar: Mision!
  }
`;

function tokenValido(recibido, esperado) {
  const izquierda = Buffer.from(String(recibido || ""));
  const derecha = Buffer.from(String(esperado || ""));
  if (derecha.length === 0 || izquierda.length !== derecha.length) return false;
  return crypto.timingSafeEqual(izquierda, derecha);
}

function bearer(authorization) {
  const valor = String(authorization || "");
  return valor.startsWith("Bearer ") ? valor.slice(7) : "";
}

function exigirToken(authorization, esperado) {
  if (!tokenValido(bearer(authorization), esperado)) {
    const error = new Error("token");
    error.extensions = { code: "UNAUTHENTICATED" };
    throw error;
  }
}

function comoMision(datos) {
  const origen = datos && typeof datos === "object" ? datos : {};
  const lugares = Array.isArray(origen.lugares) ? origen.lugares.filter((n) => Number.isInteger(n)) : [];
  const nombres = Array.isArray(origen.nombres) ? origen.nombres.map((n) => String(n)) : [];
  return {
    estado: String(origen.estado || "quieto"),
    detalle: String(origen.detalle || ""),
    lugares,
    nombres,
    lugar: String(origen.lugar || ""),
    accion: String(origen.accion || ""),
  };
}

function robotListo(estado) {
  return !!(estado
    && estado.version === "1.6"
    && estado.enlace === 1
    && estado.imu === 1
    && estado.lidar === 1
    && estado.configurada === 1
    && estado.permiso === 1);
}

async function obtenerJson(url, opciones, fetcher) {
  const { timeout, ...resto } = opciones || {};
  const control = new AbortController();
  const reloj = setTimeout(() => control.abort(), timeout || 2500);
  try {
    const respuesta = await fetcher(url, { ...resto, signal: control.signal });
    const texto = await respuesta.text();
    let datos = null;
    if (texto) {
      try { datos = JSON.parse(texto); } catch (_error) { datos = null; }
    }
    return { ok: respuesta.ok, status: respuesta.status, datos };
  } catch (_error) {
    return { ok: false, status: 0, datos: null };
  } finally {
    clearTimeout(reloj);
  }
}

function crearResolvers(env, fetcher) {
  const token = env.TOKEN || "";
  const exigirGpu = env.EXIGIR_GPU !== "0";
  const destino = {
    llm: (env.LLM_URL || "").replace(/\/$/, ""),
    vlm: (env.VLM_URL || "").replace(/\/$/, ""),
    vla: (env.VLA_URL || "").replace(/\/$/, ""),
    gpu: (env.GPU_URL || "").replace(/\/$/, ""),
    robot: (env.ROBOT_URL || "").replace(/\/$/, ""),
    orquestador: (env.ORQUESTADOR_URL || "").replace(/\/$/, ""),
  };
  const cabeceraInterna = { "X-Entrada": "graphql", Authorization: "Bearer " + token };

  async function leerServicio(nombre, url) {
    if (!url) return { nombre, sano: false, detalle: "apagado" };
    const respuesta = await obtenerJson(url + "/salud", { timeout: 2500 }, fetcher);
    const sano = !!(respuesta.ok && respuesta.datos && respuesta.datos.sano);
    return { nombre, sano, detalle: sano ? String(respuesta.datos.detalle || "listo") : "apagado" };
  }

  async function leerRobot() {
    let respuesta = await obtenerJson(destino.robot + "/estado", { timeout: 2500 }, fetcher);
    if (respuesta.datos && !respuesta.datos.operador) {
      await new Promise((resolver) => setTimeout(resolver, 400));
      respuesta = await obtenerJson(destino.robot + "/estado", { timeout: 2500 }, fetcher);
    }
    const sano = robotListo(respuesta.datos);
    return { nombre: "robot", sano, detalle: sano ? "carro listo" : "carro sin permiso o sin enlace" };
  }

  async function consultarSalud() {
    const pedidos = [
      leerServicio("llm", destino.llm),
      leerServicio("vlm", destino.vlm),
      leerServicio("vla", destino.vla),
      leerRobot(),
    ];
    if (exigirGpu) pedidos.push(leerServicio("gpu", destino.gpu));
    const servicios = await Promise.all(pedidos);
    return { listo: servicios.every((servicio) => servicio.sano), servicios };
  }

  async function leerMision() {
    const respuesta = await obtenerJson(destino.orquestador + "/api/mision", {
      headers: cabeceraInterna,
      timeout: 2500,
    }, fetcher);
    if (!respuesta.ok || !respuesta.datos) {
      return comoMision({ estado: "detenida", detalle: "No pude leer la mision" });
    }
    return comoMision(respuesta.datos);
  }

  return {
    Query: {
      salud: () => consultarSalud(),
      mision: () => leerMision(),
    },
    Mutation: {
      iniciar: async (_padre, args, contexto) => {
        exigirToken(contexto.authorization, token);
        const texto = typeof args.texto === "string" ? args.texto.trim() : "";
        if (!texto || texto.length > 500) {
          throw new Error("texto");
        }
        const salud = await consultarSalud();
        if (!salud.listo) {
          const faltan = salud.servicios.filter((servicio) => !servicio.sano).map((servicio) => servicio.nombre);
          throw new Error("Faltan servicios: " + faltan.join(", "));
        }
        const respuesta = await obtenerJson(destino.orquestador + "/api/mision", {
          method: "POST",
          headers: { ...cabeceraInterna, "Content-Type": "application/json" },
          body: JSON.stringify({ texto }),
          timeout: 4000,
        }, fetcher);
        if (!respuesta.ok) {
          const detalle = respuesta.datos && respuesta.datos.detalle ? respuesta.datos.detalle : "mision rechazada";
          throw new Error(detalle);
        }
        return leerMision();
      },
      parar: async (_padre, _args, contexto) => {
        exigirToken(contexto.authorization, token);
        await obtenerJson(destino.orquestador + "/api/parar", {
          method: "POST",
          headers: cabeceraInterna,
          timeout: 2500,
        }, fetcher);
        return leerMision();
      },
    },
  };
}

module.exports = { typeDefs, crearResolvers, tokenValido, bearer, robotListo, comoMision };
