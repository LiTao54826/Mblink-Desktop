import json
browser=json.load(open('tests/dom_render_comparison/reference_data/browser_new.json'))
next_data=browser['elements']['FORM-002-button1']
print(next_data)

