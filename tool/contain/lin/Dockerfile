FROM debian:bookworm-slim
COPY ../../.. /app/watcher
RUN apt-get update --allow-releaseinfo-change -y -qq && apt-get install -y -qq llvm clang gcc g++ cmake ninja-build
ENTRYPOINT ["bash"]