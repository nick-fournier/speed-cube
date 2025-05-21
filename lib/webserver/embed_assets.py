import sys
import re

template_path, css_path, js_path, output_path = sys.argv[1:]

with open(template_path, "r", encoding="utf-8") as f:
    html = f.read()

with open(css_path, "r", encoding="utf-8") as f:
    css = f.read()
    css = "<style>" + css + "</style>"

with open(js_path, "r", encoding="utf-8") as f:
    js = f.read()
    js = "<script>" + js + "</script>"

css_placeholder = """<link rel="stylesheet" href="uPlot.min.css">"""
js_placeholder = """<script src="uPlot.iife.min.js"></script>"""


html = html.replace(css_placeholder, css)
html = html.replace(js_placeholder, js)

# Remove any local ip addresses
pattern = r"http://(?:192\.168|10(?:\.\d{1,3}){1}|172\.(?:1[6-9]|2[0-9]|3[0-1]))(?:\.\d{1,3}){2}"
html = re.sub(pattern, "", html)

with open(output_path, "w", encoding="utf-8") as f:
    f.write(html)
