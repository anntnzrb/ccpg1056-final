FROM alpine:latest

RUN apk add --no-cache gcc musl-dev make

WORKDIR /app

CMD ["sh", "-c", "make clean && make samples"]
