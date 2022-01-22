#!/usr/bin/python
import xml.sax
import re
 
class CDROIDHandler( xml.sax.ContentHandler ):
    def __init__(self):
       self.CurrentData = ""
       self.iddict={}
       self.autoid=1000

    def startElement(self, tag, attributes):
       for attr in attributes.getNames():
           if ':id' in attr:
              idname = attributes.get(attr)
              #print attr+':'+idname
              self.id2Dict(idname)
 
    def id2Dict(self,name):
        pos=name.find('/');
        if pos>0:
            name=name[pos+1:]
        if name[0].isalpha() or (name[0]=='_'):
            self.iddict[name]=self.autoid
            self.autoid=self.autoid+1

    def dict2RH(self,filepath):
        print self.iddict
        fr=open(filepath,"w")
        fr.write("#pragma once\n\n")
        fr.write("/*Generated by machine ,Do not edit!!!*/\n\n")
        fr.write("namespace cdroid{\n\n")
        fr.write("class R{\n")
        fr.write("public:\n\n")
        fr.write("    enum id {\n")
        dsize =len(self.iddict)
        i=0
        for k in self.iddict.keys():
            fr.write("        ");
            fr.write(k+"="+str(self.iddict[k]))
            if(i<dsize-1):
                fwrite(",")
            fr.write("\n")
            i+=1
        fr.write("    };//endof enum id\n\n")
        fr.write("};//endof class R\n\n")
        fr.write("};//endof namespace\n\n")
        fr.close()

    def dict2IDResource(self,filepath):
        fr=open(filepath,"w")
        fr.write("<resource xmlns:android=\"http://schemas.android.com/apk/res/android\">\n")
        for k,v in self.iddict.items():
            fr.write("  <id name=\""+k+"\">"+str(v)+"</id>\n")
        fr.write("</resource>\n\n")
        fr.close();

if ( __name__ == "__main__"):
   
   parser = xml.sax.make_parser()
   # turn off namepsaces
   parser.setFeature(xml.sax.handler.feature_namespaces, 0)
 
   Handler = CDROIDHandler()
   parser.setContentHandler( Handler )
   
   parser.parse("/home/houzh/Miniwin/apps/uidemo/assets/layout/main.xml")
   Handler.dict2RH("./R.h")
   Handler.dict2IDResource("./ID.xml")

