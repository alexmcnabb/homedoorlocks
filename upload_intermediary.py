if __name__ == "__main__":
    # This section gets called in place of the uploader to remove the -D arg and call avrdude
    import sys
    import os

    print("Ran Upload Intermediary Script") # (This prints out after the upload finishes for some reason)

    args = sys.argv
    args.remove("-D")
    args[0] = "avrdude"
    print(f"Launching avrdude with {args}")

    os.system(" ".join(args))

else:
    # This part gets injected with extra_scripts to call the above section
    Import("env", "projenv")
    from pprint import pprint

    def before_upload(source, target, env):
        print("Running before_upload command to inject upload_intermediary script")
        old_flags = env["UPLOADERFLAGS"]
        env["UPLOADER"] = f"{env['PYTHONEXE']} upload_intermediary.py"
    env.AddPreAction("upload", before_upload)



