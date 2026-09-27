"""只验证 XML 8.3 的 TX 行为、安全边界、静态诊断与标准解析器互通。"""

from pathlib import Path
import os
import subprocess
import sys
import xml.etree.ElementTree as etree


root = Path(__file__).resolve().parents[1]
output = root / "tx_build"
sys.stdout.reconfigure(encoding="utf-8")


def run(command):
    environment = os.environ.copy()
    environment["PATH"] = str(root / "tx") + os.pathsep + environment["PATH"]
    result = subprocess.run([str(part) for part in command], cwd=root,
                            env=environment, capture_output=True, timeout=180)
    data = result.stdout + result.stderr
    try:
        message = data.decode("utf-8")
    except UnicodeDecodeError:
        message = data.decode("gb18030", errors="replace")
    if result.returncode:
        raise AssertionError(f"exit={result.returncode}\n{message}")
    return message.strip()


def main():
    output.mkdir(exist_ok=True)
    for source in ("examples/xml_stream.tx", "tests/xml/behavior.tx"):
        program = output / f"xml_{Path(source).stem}.exe"
        run([root / "tx/txc.exe", root / source, "-o", program])
        print(run([program]))
    document = etree.parse(output / "xml_stream_check.xml")
    root_node = document.getroot()
    assert root_node.tag == "{urn:catalog}catalog"
    assert root_node.attrib["{http://www.w3.org/XML/1998/namespace}lang"] == "zh"
    assert root_node[0].tag == "{urn:catalog}item"
    assert root_node[0].text == "甲 & 乙"
    print("XML_PYTHON_INTEROP_OK")
    result = subprocess.run([str(root / "tx/txc.exe"), "check",
                             str(root / "tests/xml/wrong_type.tx")],
                            cwd=root, capture_output=True, timeout=15)
    assert result.returncode != 0 and b"wrong_type.tx:6:" in result.stderr
    print("XML_STATIC_DIAGNOSTIC_OK")


if __name__ == "__main__":
    main()
