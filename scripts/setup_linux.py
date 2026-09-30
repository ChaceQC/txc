"""为 Ubuntu 24.04 x86_64 安装构建依赖；不读取或输出配置中的凭据。"""

from pathlib import Path
import os
import platform
import subprocess
import tempfile
import urllib.request


def run(*arguments):
    subprocess.run(arguments, check=True, env={**os.environ, "DEBIAN_FRONTEND": "noninteractive"})


def main():
    if platform.system() != "Linux" or platform.machine() != "x86_64" or os.geteuid() != 0:
        raise SystemExit("请在 Ubuntu 24.04 x86_64 上以 root 运行此脚本")
    release = dict(line.split("=", 1) for line in Path("/etc/os-release").read_text(encoding="utf-8").splitlines() if "=" in line)
    if release.get("ID", "").strip('"') != "ubuntu" or release.get("VERSION_ID", "").strip('"') != "24.04":
        raise SystemExit("当前依赖安装脚本以 Ubuntu 24.04 为基线")
    run("apt-get", "update", "-qq")
    run("apt-get", "install", "-y", "-qq", "ca-certificates", "curl", "gnupg")
    key = Path("/usr/share/keyrings/txc-postgresql.asc")
    key.write_bytes(urllib.request.urlopen("https://www.postgresql.org/media/keys/ACCC4CF8.asc", timeout=60).read())
    Path("/etc/apt/sources.list.d/txc-postgresql.list").write_text(
        "deb [arch=amd64 signed-by=/usr/share/keyrings/txc-postgresql.asc] https://apt.postgresql.org/pub/repos/apt noble-pgdg main\n", encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="txc-dependencies-") as temporary:
        package = Path(temporary) / "packages-microsoft-prod.deb"
        package.write_bytes(urllib.request.urlopen(
            "https://packages.microsoft.com/config/ubuntu/24.04/packages-microsoft-prod.deb", timeout=60).read())
        run("dpkg", "-i", str(package))
    run("apt-get", "update", "-qq")
    run("apt-get", "install", "-y", "-qq", "build-essential", "cmake", "ninja-build",
        "clang-18", "lld-18", "llvm-18", "ccache", "pkg-config", "patchelf",
        "libicu-dev", "libpcre2-dev", "libsodium-dev", "libpq-dev", "libssl-dev",
        "libcurl4-openssl-dev", "libnghttp2-dev", "libxml2-dev", "libargon2-dev",
        "libc-ares-dev", "libmsquic=2.6.1", "zlib1g-dev", "python3")


if __name__ == "__main__":
    main()
