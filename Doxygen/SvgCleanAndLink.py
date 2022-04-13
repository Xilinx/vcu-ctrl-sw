#!/usr/bin/env python3

import sys
import xml.etree.ElementTree as ET

namespaces = { 'SVG': 'http://www.w3.org/2000/svg', 'HTML': 'http://www.w3.org/1999/xhtml'}

def main():
  links = {}

  ET.register_namespace("", namespaces['SVG'])

  if sys.argv[3] :
    get_function_links(sys.argv[3], links)

  cleanup_svg(sys.argv[1], sys.argv[2], links);

def cleanup_svg(svgfilein, svgfileout, links):
  xmltree = ET.parse(svgfilein)
  root = xmltree.getroot()

  for defs in root.findall('SVG:defs', namespaces):
    root.remove(defs)

  parse_g(root, links)

  xmltree.write(svgfileout)

def parse_g(tree, links):
  for g in tree.findall('SVG:g', namespaces):
    if 'font-family' in g.attrib:
      g.attrib['font-family'] = g.attrib['font-family'].replace(' embedded', '')
    parse_g(g, links)

  for text in tree.findall('SVG:text', namespaces):
    for tspan in text.findall('SVG:tspan', namespaces):
      text.text = tspan.text
      if tspan.text in links:
        link = ET.Element("a", { 'href': links[tspan.text], 'target':'_top' })
        link.append(text)
        tree.append(link)
        tree.remove(text)
      text.remove(tspan)

def get_function_links(funcfile, links):
  return get_function_links_rec(ET.parse(funcfile).getroot(), links)

def get_function_links_rec(tree, links):
  for child in tree.findall('HTML:li', namespaces):
    get_link(child, links)

  for child in tree:
    get_function_links_rec(child, links)

def get_link(elem, links):
  name = elem.text
  if name != None and name[0:3] == "AL_":
    link = elem.find('HTML:a', namespaces)
    if link != None and 'href' in link.attrib :
      links[name.split('(')[0]] = link.attrib['href']

main()
