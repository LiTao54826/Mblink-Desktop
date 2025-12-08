#!/usr/bin/env python3
"""Extract browser data using Playwright and save to JSON file."""
import json
import os
from playwright.sync_api import sync_playwright

def extract_browser_data():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    html_path = os.path.join(script_dir, 'test_cases', 'index.html')
    output_path = os.path.join(script_dir, 'reference_data', 'browser_new.json')

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()
        page.set_viewport_size({"width": 785, "height": 600})
        page.goto(f'file:///{html_path}')
        page.wait_for_timeout(500)  # Wait for rendering

        # Extract data using the page's built-in function
        data = page.evaluate('() => window.DOMRenderTest.extract()')

        browser.close()

    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)

    print(f"Browser data saved to {output_path}")
    print(f"Total elements: {len(data.get('elements', {}))}")
    return data

if __name__ == '__main__':
    extract_browser_data()

