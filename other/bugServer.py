import hashlib
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import json
import os

file_path = os.path.dirname(os.path.realpath(__file__))

class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
    
    def do_GET(self):
        self._set_headers()
        self.wfile.write("huh?")
    
    def do_HEAD(self):
        self._set_headers()
    
    def do_POST(self):
        # Read Bug number
        bugNumber = 0
        bugNumPath = file_path + '/bugNum.dat'
        if not (os.path.exists(bugNumPath)):
            f = open(bugNumPath, 'w')
            f.write('0')
            f.close()
        else:
            f = open(bugNumPath, 'r+')
            bugNumber = int(f.read())
            f.write(str(bugNumber + 1))
            f.close()
        
        
        
        
        # Doesn't do anything with posted data
        self._set_headers()
        self.wfile.write("success")
        content_len = int(self.headers.getheader('content-length', 0))
        print "Receiving post of length:", content_len / 1000000.0, "MB"
        post_body = self.rfile.read(content_len)
        jdata = json.loads(post_body)
        fileData = "Version: " + jdata["version"]
        fileData = "\nDevice: " + jdata["device"]
        fileData += "\nIs System Application: " + str(jdata["isSystem"])
        fileData += "\nDescription: " + jdata["description"]
        if ("game" in jdata):
            fileData += "\nGame Name: " + jdata["game"]
            fileData += "\nGame Code: " + jdata["gameCode"]
        if ("save" in jdata):
            save = jdata["save"]
            saveHash = hashlib.sha1(save).hexdigest()
            f = open(file_path + '/saves/' + saveHash + '.dsv', 'w')
            f.write(save.decode('base64'))
            f.close()
            fileData += "\nSave: " + saveHash  + ".dsv"
            jdata["save"] = saveHash
        if ("image" in jdata):
            image = jdata["image"]
            imageHash = hashlib.sha1(image).hexdigest()
            f = open(file_path + '/images/' + imageHash + '.jpg', 'w')
            f.write(image.decode('base64'))
            f.close()
            fileData += "\nImage: " + imageHash + ".jpg"
            jdata["image"] = imageHash
        fileData += "\n\nJSON Dump: " + json.dumps(jdata)
        print fileData
        f = open(file_path + '/bugs/bug_'+str(bugNumber)+'.txt', 'w')
        f.write(fileData)
        f.close()
        print "Post Success"

def run(server_class=HTTPServer, handler_class=S, port=6768):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print 'Starting httpd...'
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv
    
    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()