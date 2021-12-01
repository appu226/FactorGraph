def getTestPath():
  import pathlib
  import os
  return os.path.join(pathlib.Path(__file__).parent.resolve(), "data")
