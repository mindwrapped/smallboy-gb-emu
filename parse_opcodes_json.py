# Parse opcodes from opcodes.json to csv
import json

with open('opcodes.json') as f:
    data = json.load(f)

for d in list(data.keys()):
    # print(d)

    unp = data[d]
    for opc in list(unp.keys()):
        # print(opc)
        if d == 'unprefixed':
            nm = ''
        else:
            nm = 'CB '
        opcd = unp[opc]
        nm += opcd['mnemonic']
        bytez = opcd['bytes']
        cyc = opcd['cycles'][0]
        fZ = opcd['flags']['Z']
        fN = opcd['flags']['N']
        fH = opcd['flags']['H']
        fC = opcd['flags']['C']
        for operand in opcd['operands']:
            nm += ' ' + operand['name']

        f = open('opcodes.csv', 'a')
        f.write("{},{},{},{},{},{},{},{}\n".format(opc,nm, bytez,cyc,fZ,fN,fH,fC))
        f.close()
