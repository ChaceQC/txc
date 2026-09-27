"""第 9.7 节的标准向量、跨库、拒绝路径和资源清理定向验收。"""

from pathlib import Path
from struct import pack, unpack_from
import os
import random
import subprocess
import sys
import tempfile

from argon2 import PasswordHasher, Type
from argon2.low_level import hash_secret
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ed25519, x25519
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.hkdf import HKDF


root = Path(__file__).resolve().parents[1]
sys.stdout.reconfigure(encoding="utf-8")
message = bytes((index * 37 + 11) % 256 for index in range(2 * 65536 + 37))
key = bytes(range(32))
aad = "TX acceptance context".encode("utf-8")
key_id = b"interop-key"


def run(*command, timeout=180):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(item) for item in command], cwd=root,
                            env=environment, capture_output=True,
                            timeout=timeout)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{output}")
    return output.strip()


def make_txcg(nonce):
    header = b"TXCG\x01\x01" + nonce
    return header + AESGCM(key).encrypt(nonce, message, header + aad)


def read_txcg(encoded):
    assert encoded[:6] == b"TXCG\x01\x01"
    return AESGCM(key).decrypt(encoded[6:18], encoded[18:],
                               encoded[:18] + aad)


def file_key(salt):
    return HKDF(algorithm=hashes.SHA256(), length=32, salt=salt,
                info=b"TXCF-1" + key_id).derive(key)


def make_txcf(salt):
    header = b"TXCF\x01\x01" + pack(">I", 65536) + salt + bytes([len(key_id)]) + key_id
    aead = AESGCM(file_key(salt))
    result = bytearray(header)
    for sequence, start in enumerate(range(0, len(message), 65536)):
        block = message[start:start + 65536]
        record = pack(">IBI", sequence, 0, len(block))
        nonce = b"\0" * 8 + pack(">I", sequence)
        result.extend(record)
        result.extend(aead.encrypt(nonce, block, header + aad + record))
    sequence = (len(message) + 65535) // 65536
    record = pack(">IBI", sequence, 1, 0)
    result.extend(record)
    result.extend(aead.encrypt(b"\0" * 8 + pack(">I", sequence), b"",
                               header + aad + record))
    return bytes(result)


def read_txcf(encoded):
    assert encoded[:6] == b"TXCF\x01\x01"
    assert unpack_from(">I", encoded, 6)[0] == 65536
    salt = encoded[10:26]
    length = encoded[26]
    assert encoded[27:27 + length] == key_id
    header = encoded[:27 + length]
    aead = AESGCM(file_key(salt))
    offset = len(header)
    chunks = []
    sequence = 0
    while True:
        assert offset + 25 <= len(encoded)
        record = encoded[offset:offset + 9]
        actual, final, size = unpack_from(">IBI", record)
        assert actual == sequence and final in (0, 1)
        assert size <= 65536 and (size == 0) == bool(final)
        offset += 9
        cipher = encoded[offset:offset + size + 16]
        assert len(cipher) == size + 16
        nonce = b"\0" * 8 + pack(">I", sequence)
        chunks.append(aead.decrypt(nonce, cipher, header + aad + record))
        offset += len(cipher)
        if final:
            assert offset == len(encoded)
            return b"".join(chunks)
        sequence += 1


def write_fixtures(directory):
    (directory / "message.bin").write_bytes(message)
    (directory / "aes.key").write_bytes(key)
    (directory / "external.txcg").write_bytes(make_txcg(bytes(range(12))))
    # NIST SP 800-38D AES-256-GCM 零密钥向量；TXCG 的头部另作为 AAD 认证。
    zero_cipher = AESGCM(bytes(32)).encrypt(bytes(12), bytes(16), b"")
    assert zero_cipher.hex() == (
        "cea7403d4d606b6e074ec5d3baf39d18"
        "d0d1c8a799996bf0265b98b5d48ab919")
    nist_header = b"TXCG\x01\x01" + bytes(12)
    (directory / "nist.txcg").write_bytes(nist_header + AESGCM(bytes(32)).encrypt(
        bytes(12), bytes(16), nist_header))
    external_file = make_txcf(bytes(range(16)))
    (directory / "external.txcf").write_bytes(external_file)
    corrupted = bytearray(external_file)
    corrupted[-1] ^= 1
    (directory / "bad.txcf").write_bytes(corrupted)
    generator = random.Random(0x97C0)
    for index in range(32):
        for suffix, source in (("txcg", make_txcg(bytes(range(12)))),
                               ("txcf", external_file)):
            corrupted = bytearray(source)
            position = generator.randrange(len(corrupted))
            corrupted[position] ^= 1 << generator.randrange(8)
            (directory / f"fuzz-{index}.{suffix}").write_bytes(corrupted)

    signing = ed25519.Ed25519PrivateKey.from_private_bytes(bytes(range(32, 64)))
    other_signing = ed25519.Ed25519PrivateKey.from_private_bytes(bytes(range(64, 96)))
    (directory / "ed.seed").write_bytes(bytes(range(32, 64)))
    (directory / "external.ed.public").write_bytes(
        other_signing.public_key().public_bytes(
            serialization.Encoding.Raw, serialization.PublicFormat.Raw))
    (directory / "external.ed.signature").write_bytes(other_signing.sign(message))

    alice = bytes.fromhex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a")
    bob = bytes.fromhex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb")
    (directory / "x.private").write_bytes(alice)
    (directory / "external.x.public").write_bytes(
        x25519.X25519PrivateKey.from_private_bytes(bob).public_key().public_bytes(
            serialization.Encoding.Raw, serialization.PublicFormat.Raw))

    phc = hash_secret(b"interop password", bytes(range(16)), time_cost=2,
                      memory_cost=19456, parallelism=1, hash_len=32,
                      type=Type.ID, version=19)
    (directory / "external.argon2.phc").write_bytes(phc)
    return signing, alice, bob


def check_interop(directory, signing, alice, bob):
    first = (directory / "tx.txcg").read_bytes()
    second = (directory / "tx-second.txcg").read_bytes()
    assert first[6:18] != second[6:18]
    assert read_txcg(first) == message
    assert read_txcg(second) == message
    first_file = (directory / "tx.txcf").read_bytes()
    second_file = (directory / "tx-second.txcf").read_bytes()
    assert first_file[10:26] != second_file[10:26]
    assert read_txcf(first_file) == message
    assert read_txcf(second_file) == message
    assert (directory / "restored.bin").read_bytes() == b"old target remains"
    assert not list(directory.glob("*.tx-crypt-*"))

    public = (directory / "tx.ed.public").read_bytes()
    signature = (directory / "tx.ed.signature").read_bytes()
    assert public == signing.public_key().public_bytes(
        serialization.Encoding.Raw, serialization.PublicFormat.Raw)
    ed25519.Ed25519PublicKey.from_public_bytes(public).verify(signature, message)
    try:
        ed25519.Ed25519PublicKey.from_public_bytes(public).verify(signature,
                                                                  message + b"x")
    except InvalidSignature:
        pass
    else:
        raise AssertionError("changed Ed25519 message was accepted")

    private = x25519.X25519PrivateKey.from_private_bytes(alice)
    peer = x25519.X25519PrivateKey.from_private_bytes(bob)
    assert (directory / "tx.x.public").read_bytes() == private.public_key().public_bytes(
        serialization.Encoding.Raw, serialization.PublicFormat.Raw)
    shared = private.exchange(peer.public_key())
    assert shared.hex() == "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742"
    expected = HKDF(algorithm=hashes.SHA256(), length=32,
                    salt=b"exchange-salt", info=b"interop-v1").derive(shared)
    assert (directory / "tx.x.derived").read_bytes() == expected
    assert PasswordHasher().verify(
        (directory / "tx.argon2.phc").read_text(encoding="utf-8"),
        "interop password")


def compile_native(directory):
    executable = directory / "random_failure.exe"
    run("g++", "-std=c++23", "-O2", "-finput-charset=UTF-8",
        "-fexec-charset=UTF-8", "-Isrc",
        root / "tests/crypto/random_failure_native.cpp",
        root / "tx/libtxstdlib.a", "-Wl,--wrap=psa_generate_random",
        "-Ltx/link", "-lcrypt32", "-lncrypt", "-ladvapi32", "-lbcrypt",
        "-lwinhttp", "-lws2_32", "-lshell32", "-luser32", "-liconv",
        "-o", executable)
    print(run(executable, directory))


def main():
    with tempfile.TemporaryDirectory(prefix="tx-crypto-acceptance-") as location:
        directory = Path(location)
        signing, alice, bob = write_fixtures(directory)
        program = directory / "interop.exe"
        run(root / "tx/txc.exe", root / "tests/crypto/acceptance_interop.tx",
            "-o", program)
        print(run(program, directory))
        check_interop(directory, signing, alice, bob)
        print("CRYPTO_CROSSLIB_OK")
        compile_native(directory)
        for source, marker in (("behavior.tx", "CRYPTO_OK"),
                               ("public_key.tx", "PUBLIC_KEY_OK")):
            executable = directory / (source + ".exe")
            run(root / "tx/txc.exe", root / "tests/crypto" / source,
                "-o", executable)
            assert marker in run(executable)
            print(marker)
        print(run(sys.executable, root / "scripts/check_x509.py"))
    print("CRYPTO_ACCEPTANCE_OK")


if __name__ == "__main__":
    main()
