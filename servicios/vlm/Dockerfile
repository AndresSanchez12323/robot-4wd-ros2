FROM python:3.12-slim
WORKDIR /app
COPY comun /app/comun
COPY vlm /app/vlm
ENV PYTHONPATH=/app PYTHONDONTWRITEBYTECODE=1
USER nobody
EXPOSE 8080
CMD ["python3", "-u", "vlm/app.py"]
