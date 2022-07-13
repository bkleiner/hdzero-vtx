Import("env")

SDCC_OPTS = ["--model-large", "--opt-code-speed", "--no-pack-iram"]

env.Append(
    CFLAGS=SDCC_OPTS,
    LINKFLAGS=SDCC_OPTS
)
