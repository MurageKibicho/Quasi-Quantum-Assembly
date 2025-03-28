import safetensors
import struct
import tiktoken
import torch
from torch.nn import functional as F

f = open("data", "rb")
token_bytes = f.read(65 * 2)
tokens = struct.unpack("65H", token_bytes)
# print(tokens)
# print(len(tokens))

enc = tiktoken.encoding_for_model("gpt2")
# print(enc.decode(list(tokens[:65])))

f = open("model.safetensors", "rb")
fts = safetensors.deserialize(f.read())
params = dict()
for ft in fts:
    name = ft[0]
    data = ft[1]["data"]
    shape = ft[1]["shape"]
    params[name] = torch.frombuffer(data, dtype=torch.float32, requires_grad=True).reshape(shape)
    params[name].retain_grad()

x = torch.tensor(tokens[:64], dtype=torch.long)
y_true = torch.tensor(tokens[1:65], dtype=torch.long)

input_size = len(x)
d_model = 768
d_k = 64

wte_out = F.embedding(
    input=x,
    weight=params["wte.weight"])
wpe_out = F.embedding(
    input=torch.arange(0, len(x), dtype=torch.long),
    weight=params["wpe.weight"])
embedding_out = wte_out + wpe_out

for layer_i in range(12):
    layer_in = embedding_out if layer_i == 0 else res_2_out
    ln_1_out = F.layer_norm(
        input=layer_in,
        normalized_shape=[d_model],
        weight=params[f"h.{layer_i}.ln_1.weight"],
        bias=params[f"h.{layer_i}.ln_1.bias"],
        eps=1e-5)
    attn_c_attn_out = F.linear(
        input=ln_1_out,
        weight=params[f"h.{layer_i}.attn.c_attn.weight"].transpose(0, 1),
        bias=params[f"h.{layer_i}.attn.c_attn.bias"])
    q, k, v = attn_c_attn_out.split(d_model, dim=1)
    attn_z_out = torch.zeros([input_size, d_model])
    for head_i in range(12):
        a = q[:,head_i*d_k:(head_i+1)*d_k] @ k[:,head_i*d_k:(head_i+1)*d_k].transpose(0, 1) * torch.rsqrt(torch.tensor(d_k, dtype=torch.float32))
        mask = torch.triu(torch.ones_like(a, dtype=torch.bool), diagonal=1)
        a = torch.masked_fill(a, mask, -torch.inf)
        s = F.softmax(a, dim=-1)
        z = s @ v[:,head_i*d_k:(head_i+1)*d_k]
        attn_z_out[:,head_i*d_k:(head_i+1)*d_k] = z
    attn_c_proj_out = F.linear(
        input=attn_z_out,
        weight=params[f"h.{layer_i}.attn.c_proj.weight"].transpose(0, 1),
        bias=params[f"h.{layer_i}.attn.c_proj.bias"])
    res_1_out = layer_in + attn_c_proj_out

    ln_2_out = F.layer_norm(
        input=res_1_out,
        normalized_shape=[d_model],
        weight=params[f"h.{layer_i}.ln_2.weight"],
        bias=params[f"h.{layer_i}.ln_2.bias"],
        eps=1e-5)
    mlp_c_fc_out = F.linear(
        input=ln_2_out,
        weight=params[f"h.{layer_i}.mlp.c_fc.weight"].transpose(0, 1),
        bias=params[f"h.{layer_i}.mlp.c_fc.bias"])
    mlp_gelu_out = F.gelu(mlp_c_fc_out)
    mlp_c_proj_out = F.linear(
        input=mlp_gelu_out,
        weight=params[f"h.{layer_i}.mlp.c_proj.weight"].transpose(0, 1),
        bias=params[f"h.{layer_i}.mlp.c_proj.bias"])
    res_2_out = res_1_out + mlp_c_proj_out

ln_f_out = F.layer_norm(
    input=res_2_out,
    normalized_shape=[d_model],
    weight=params["ln_f.weight"],
    bias=params["ln_f.bias"],
    eps=1e-5)

y_pred = F.linear(
    input=ln_f_out,
    weight=params["wte.weight"])

loss = F.cross_entropy(y_pred, y_true)
assert(abs(loss - 4.1333) < 1e-3)

# loss.backward()
# print(params["wte.weight"].grad.sum())
