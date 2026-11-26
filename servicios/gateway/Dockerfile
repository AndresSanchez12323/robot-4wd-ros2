FROM node:22-alpine
WORKDIR /app
COPY gateway/package.json gateway/package-lock.json ./
RUN npm ci --omit=dev
COPY gateway/index.js gateway/resolvers.js gateway/pagina.html ./
USER node
EXPOSE 4000
CMD ["node", "index.js"]
