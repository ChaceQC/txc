"""定向验证共用的平台路径；TXC_TOOL_DIR 可选择解包后的工具链。"""

from pathlib import Path
import os
import subprocess


ROOT = Path(__file__).resolve().parents[1]
TOOL_DIR = Path(os.environ.get("TXC_TOOL_DIR", ROOT / "tx")).resolve()
EXE_SUFFIX = ".exe" if os.name == "nt" else ""
TXC = TOOL_DIR / ("txc" + EXE_SUFFIX)
CREATE_FLAGS = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0


def environment():
    values = os.environ.copy()
    values["PATH"] = str(TOOL_DIR) + os.pathsep + values.get("PATH", "")
    if os.name != "nt":
        values["LD_LIBRARY_PATH"] = str(TOOL_DIR / "lib") + os.pathsep + values.get("LD_LIBRARY_PATH", "")
    return values
