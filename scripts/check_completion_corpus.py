"""有界、固定种子的输入变异回归；不是覆盖引导或长期模糊测试。"""

from pathlib import Path
import json
import random
import tempfile

from check_completion_chains import run
from check_tls_stream import compile_case, environment

ROOT = Path(__file__).resolve().parent.parent
SEED = 20260930


def main():
    rng = random.Random(SEED)
    seeds = ['{"name":"中文","items":[1,true,null]}', '<record a="b">text</record>',
             'name,amount\r\nexample,42\r\n', 'SELECT 1', '(a|b)+', '', '[' * 40]
    corpus = []
    alphabet = '{}[]<>,:"\\\r\n012ab *?()/;\x00'
    for index in range(128):
        text = seeds[index % len(seeds)]
        for _ in range(rng.randrange(1, 5)):
            position = rng.randrange(len(text) + 1)
            text = text[:position] + rng.choice(alphabet) + text[position + rng.randrange(2):]
        binary = bytes(rng.randrange(256) for _ in range(rng.randrange(1, 96)))
        corpus.append({"text": text, "binary": binary.hex()})
    env = environment()
    with tempfile.TemporaryDirectory(prefix="tx-corpus-") as location:
        directory = Path(location)
        data = directory / "corpus.json"
        data.write_text(json.dumps(corpus, ensure_ascii=False), encoding="utf-8")
        program = directory / "corpus.exe"
        compile_case(ROOT / "tests/completion/parser_corpus.tx", program, env)
        assert run([program, data], directory, env).strip() == "128"
    print(f"变异回归通过：seed={SEED}，128 个输入 × JSON/CSV/XML/CBOR/正则/SQLite prepare")


if __name__ == "__main__":
    main()
