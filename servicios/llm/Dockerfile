FROM python:3.12-slim
WORKDIR /app
COPY comun /app/comun
COPY llm /app/llm
ENV PYTHONPATH=/app PYTHONDONTWRITEBYTECODE=1
USER nobody
EXPOSE 8080
CMD ["python3", "-u", "llm/app.py"]
