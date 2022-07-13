Import("env")

env.Append(
    CFLAGS=["--model-large", "--opt-code-speed",
            "--stack-auto", "--fomit-frame-pointer"],
    LINKFLAGS=["--model-large", "--opt-code-speed", "--stack-auto",
               "--xram-loc", "0x0001", "--code-loc", "0x0"]
)
