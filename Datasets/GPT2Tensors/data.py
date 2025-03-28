import tiktoken
import struct

enc = tiktoken.get_encoding("gpt2")
with open("tiny.txt", "r") as f:
    tokens = enc.encode(f.read())

print(len(tokens))

with open("data", "wb") as f:
    for token in tokens:
        f.write(struct.pack("H", token))
