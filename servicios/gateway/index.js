/** Puerta GraphQL. La pagina y las consultas entran por aqui; los demas servicios no se publican. */

const express = require("express");
const { ApolloServer } = require("@apollo/server");
const { expressMiddleware } = require("@as-integrations/express4");
const { ApolloServerPluginLandingPageLocalDefault } = require("@apollo/server/plugin/landingPage/default");
const fs = require("fs");
const path = require("path");
const { typeDefs, crearResolvers } = require("./resolvers");

const PUERTO = Number(process.env.PUERTO || 4000);
const TOKEN = process.env.TOKEN || "";
const pagina = fs.readFileSync(path.join(__dirname, "pagina.html"));

async function main() {
  if (!TOKEN || !process.env.ORQUESTADOR_URL || !process.env.LLM_URL || !process.env.ROBOT_URL) {
    throw new Error("Faltan TOKEN, ORQUESTADOR_URL, LLM_URL o ROBOT_URL");
  }
  const server = new ApolloServer({
    typeDefs,
    resolvers: crearResolvers(process.env, (...args) => fetch(...args)),
    plugins: [ApolloServerPluginLandingPageLocalDefault({ embed: true })],
  });
  await server.start();
  const app = express();
  app.disable("x-powered-by");
  app.get("/", (_req, res) => {
    res.set("Cache-Control", "no-store");
    res.type("html").send(pagina);
  });
  app.use("/graphql", express.json({ limit: "32kb" }), expressMiddleware(server, {
    context: async ({ req }) => ({ authorization: req.headers.authorization || "" }),
  }));
  app.listen(PUERTO, "0.0.0.0", () => {
    console.log("GraphQL listo en http://localhost:" + PUERTO + "/graphql");
  });
}

if (require.main === module) {
  main().catch((error) => {
    console.error(error.message);
    process.exit(1);
  });
}
