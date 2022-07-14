Import("env")

SDCC_OPTS = ["--model-large", "--stack-auto"]

env.Append(
    CFLAGS=SDCC_OPTS,
    LINKFLAGS=SDCC_OPTS
)
