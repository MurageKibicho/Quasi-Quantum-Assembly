import tiktoken
import struct

enc = tiktoken.get_encoding("gpt2")
offsets_sizes = []
offset = 0
string = b""

for i in range(50257):
    offsets_sizes.append(offset)
    if i < 50256:
        string += enc.decode_single_token_bytes(i)
    else:
        string += b"<|endoftext|>"
    new_offset = len(string)
    offsets_sizes.append(new_offset - offset)
    offset = new_offset
   
with open("enc", "wb") as f:
    for i in offsets_sizes:
        f.write(struct.pack("I", i))
    a = f.tell()
    f.write(string)
    b = f.tell()
    print("a:", a, "b:", b, "b-a:", b - a)