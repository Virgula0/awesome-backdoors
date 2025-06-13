import pefile
import sys

def main():
    exports_list = []
    pe = pefile.PE(sys.argv[1])
    for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        try:
            exports_list.append(exp.name.decode('utf-8'))
        except:
            continue

    fd = open('exports.txt','w')
    for exp in exports_list:
        fd.write('#pragma comment(linker, "/export:{}={}.{}")\n'.format(exp, sys.argv[1].split(".")[0], exp))
    fd.close()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python dll_exports.py <path-to-DLL>")
    else:
        main()