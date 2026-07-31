# syntax=docker/dockerfile:1

# Stage 1: Install application dependencies into an isolated directory.
FROM python:3.12-slim AS builder

WORKDIR /build

COPY requirements.txt .
RUN pip install --no-cache-dir --prefix=/install -r requirements.txt

# Stage 2: Create the minimal runtime image.
FROM python:3.12-slim AS runtime

# Set the application directory and runtime defaults.
WORKDIR /app
ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PORT=8080

# Copy only installed dependencies from the builder stage.
COPY --from=builder /install /usr/local

# Copy application source code into the image.
COPY . .

# Document the port used by the application.
EXPOSE 8080

# Verify that the application responds successfully.
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
  CMD python -c "import os,urllib.request; urllib.request.urlopen('http://127.0.0.1:%s/health' % os.getenv('PORT', '8080'), timeout=3)" || exit 1

# Use an exec-form entrypoint while allowing arguments to be overridden.
ENTRYPOINT ["python", "-m", "app"]

# Default arguments can be replaced at runtime:
# docker run image --host 0.0.0.0 --port 8080
CMD ["--host", "0.0.0.0", "--port", "8080"]
