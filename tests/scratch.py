import json
browser = json.load(open(r"tests/dom_render_comparison/reference_data/browser_new.json"))
mbink = json.load(open(r"tests/dom_render_comparison/reference_data/mbink_new.json"))
for key in ['FORM-002-container','FORM-002-button1','FORM-002-button2']:
    print(key)
    b = browser['elements'][key]['viewport']
    m = mbink['elements'][key]['viewport']
    print(' browser:', b)
    print(' mbink  :', m)
    print()

