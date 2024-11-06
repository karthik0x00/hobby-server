FROM gcc:latest
WORKDIR /usr/src/app
RUN apt update && apt install netcat-openbsd
COPY . .
EXPOSE 9001
