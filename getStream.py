import picamera
import sys
import os
import json
from io import BytesIO
from struct import pack

configName = "config.json"
if len(sys.argv) > 1:
	configName = sys.argv[1]
sys.stderr.write("python: Opening config file " + repr(configName) + "\n")
with open(configName, 'r') as file:
        conf = json.load(file)
        file.close()
sys.stderr.write("OK\n")

sys.stderr.write("python: Opening camera instance: ")
cam = picamera.PiCamera()
cam.rotation = conf["camera"]["rotation"]
if (cam.rotation == 90) or (cam.rotation == 270):
	conf["camera"]["resolution"] = [conf["camera"]["resolution"][1], conf["camera"]["resolution"][0]]
cam.resolution = conf["camera"]["resolution"]
cam.framerate = conf["camera"]["framerate"]
cam.hflip = conf["camera"]["hflip"]
cam.vflip = conf["camera"]["vflip"]
settings = "python: resolution " + repr(cam.resolution) + "\n"
settings = settings + "python: framerate " + repr(cam.framerate) + "\n"
settings = settings + "python: rotation " + repr(cam.rotation) + "\n"
settings = settings + "python: hflip " + repr(cam.hflip) + "\n"
settings = settings + "python: vflip " + repr(cam.vflip) + "\n"
settings = settings + "python: videoPort " + repr(conf["camera"]["videoPort"]) + "\n"
data =  BytesIO()
sys.stderr.write("OK\n")
sys.stderr.write(settings)

output = sys.stdout.fileno()
for filename in enumerate(cam.capture_continuous(data, 'jpeg', conf["camera"]["videoPort"])):
	data.seek(0)
	llen = len(data.getvalue())
	os.write(output, pack("I", llen))
	os.write(output, data.getvalue())
