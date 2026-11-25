"""Coordinate Linux builds and SDK replacement; flock releases on process exit."""
from contextlib import contextmanager
import json
import os
from pathlib import Path
import sys
import uuid


@contextmanager
def operation_lock(root):
    if sys.platform != "linux":
        yield
        return
    import fcntl
    path = Path(root) / "verification/project-operation.lock"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+", encoding="utf-8") as handle:
        handle.seek(0)
        current = handle.read()
        inherited = os.environ.get("ASTROTOO_OPERATION_TOKEN")
        try:
            fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            if inherited and current == inherited:
                yield
                return
            raise RuntimeError("another build or SDK sync is running; wait for it to finish")
        token = json.dumps({"pid": os.getpid(), "id": uuid.uuid4().hex})
        handle.seek(0)
        handle.read()  # Re-read before changing the lock record.
        handle.seek(0)
        handle.truncate()
        handle.write(token)
        handle.flush()
        os.environ["ASTROTOO_OPERATION_TOKEN"] = token
        try:
            yield
        finally:
            if inherited is None:
                os.environ.pop("ASTROTOO_OPERATION_TOKEN", None)
            else:
                os.environ["ASTROTOO_OPERATION_TOKEN"] = inherited
            fcntl.flock(handle, fcntl.LOCK_UN)
