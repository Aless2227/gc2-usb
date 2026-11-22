# Standard devkitA64 environment; the delivered binary was built with the LLVM
# alternative described in docs/BUILD.md. This Docker route has not run here.
FROM devkitpro/devkita64@sha256:82575ea78651b530b2e232bb3799cfd1fe331514e053d5f724bb4b28191fb79d
WORKDIR /project
CMD ["make", "package"]
