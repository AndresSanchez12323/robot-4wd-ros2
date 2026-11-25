FROM nvidia/cuda:12.4.1-base-ubuntu22.04
RUN apt-get update \
    && apt-get install -y --no-install-recommends python3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY gpu/app.py /app/app.py
EXPOSE 8080
CMD ["python3", "-u", "/app/app.py"]
