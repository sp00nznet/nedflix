# Nedflix - NFS Video Browser
# Dockerfile for containerized deployment

FROM node:20-alpine

# Install OpenSSL for certificate generation
RUN apk add --no-cache openssl

# Labels
LABEL org.opencontainers.image.title="Nedflix"
LABEL org.opencontainers.image.description="NFS Video Browser with OAuth and HTTPS"
LABEL org.opencontainers.image.version="2.0.0"

# Create non-root user
RUN addgroup -g 1001 -S nedflix && \
    adduser -S nedflix -u 1001 -G nedflix

# Set working directory
WORKDIR /app

# Copy package files
COPY package*.json ./

# Install dependencies
RUN npm ci --only=production

# Copy application source
COPY --chown=nedflix:nedflix . .

# Create directories for certs and ensure proper permissions
RUN mkdir -p /app/certs && chown -R nedflix:nedflix /app

# Environment variables with defaults
ENV NODE_ENV=production
ENV PORT=3000
ENV HTTPS_PORT=3443
ENV NFS_MOUNT_PATH=/mnt/nfs

# Expose ports (HTTP redirect + HTTPS)
EXPOSE 3000 3443

# Switch to non-root user
USER nedflix

# Generate certs on first run if they don't exist, then start server
CMD ["sh", "-c", "if [ ! -f /app/certs/server.key ]; then node generate-certs.js; fi && node server.js"]
