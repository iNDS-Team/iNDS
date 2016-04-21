import tornado.ioloop
import tornado.web
import sqlite3
import os
import json
import hashlib
import time

file_path = os.path.dirname(os.path.realpath(__file__))

def _execute(query, values):
    dbPath = file_path + '/bugs.sqlite'
    connection = sqlite3.connect(dbPath)
    cursorobj = connection.cursor()
    try:
        cursorobj.execute(query, values)
        connection.commit()
    except Exception:
        raise
    connection.close()

class Application(tornado.web.Application):
    def __init__(self):
        handlers = [
                    (r"/iNDS/bugreport", ServiceHandler)
                    ]
        super(Application, self).__init__(handlers)


class ServiceHandler(tornado.web.RequestHandler):
    def get(self):
        self.write("Huh?")
    
    def post(self):
        # Doesn't do anything with posted data
        jdata = json.loads(self.request.body)
        
        if (not "game" in jdata):
            jdata["game"] = ""
            jdata["gameCode"] = ""
        if ("save" in jdata):
            save = jdata["save"]
            saveHash = hashlib.sha1(save).hexdigest()
            f = open(file_path + '/saves/' + saveHash + '.dsv', 'w')
            f.write(save.decode('base64'))
            f.close()
            jdata["save"] = saveHash
        else:
            jdata["save"] = ""
        if ("image" in jdata):
            image = jdata["image"]
            imageHash = hashlib.sha1(image).hexdigest()
            f = open(file_path + '/images/' + imageHash + '.jpg', 'w')
            f.write(image.decode('base64'))
            f.close()
            jdata["image"] = imageHash
        else:
            jdata["image"] = ""
                
        cur = con.cursor()
        _execute("""INSERT INTO bugs
                    (device, major, minor, isSystem, description, game, gameCode, save, image, date)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""",
                    (jdata["device"], jdata["major"], jdata["minor"], jdata["isSystem"], jdata["description"],jdata["game"], jdata["gameCode"], jdata["save"], jdata["image"], time.ctime()))
                             
                             
        print "Post Success"
        self.write('{"result": "success"}')



if __name__ == "__main__":
    print "Starting Server"
    
    # Create folders and database if the don't exist
    if not os.path.exists(file_path + '/images'):
        os.makedirs(file_path + '/images')
    if not os.path.exists(file_path + '/saves'):
        os.makedirs(file_path + '/saves')

    con = sqlite3.connect(file_path + '/bugs.sqlite')
    cursor = con.cursor()
    cursor.execute("CREATE TABLE IF NOT EXISTS bugs(Id INTEGER PRIMARY KEY, device TEXT, major TEXT, minor TEXT, isSystem INT, description TEXT, game TEXT, gameCode TEXT, save TEXT, image TEXT, date TEXT)")
    con.commit()
    
    Application().listen(6768)
    tornado.ioloop.IOLoop.instance().start()

