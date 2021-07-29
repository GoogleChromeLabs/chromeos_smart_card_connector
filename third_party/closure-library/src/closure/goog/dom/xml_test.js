/**
 * @license
 * Copyright The Closure Library Authors.
 * SPDX-License-Identifier: Apache-2.0
 */

goog.module('goog.dom.xmlTest');
goog.setTestOnly();

const TagName = goog.require('goog.dom.TagName');
const dom = goog.require('goog.dom');
const domXml = goog.require('goog.dom.xml');
const testSuite = goog.require('goog.testing.testSuite');
const userAgent = goog.require('goog.userAgent');

testSuite({
  testSerialize() {
    const doc = domXml.createDocument();
    const node = doc.createElement('root');
    doc.appendChild(node);

    const serializedNode = domXml.serialize(node);
    assertTrue(/<root ?\/>/.test(serializedNode));

    const serializedDoc = domXml.serialize(doc);
    assertTrue(/(<\?xml version="1.0"\?>)?<root ?\/>/.test(serializedDoc));
  },

  testSerializeWithActiveX() {
    // Prefer ActiveXObject if available.
    const doc = domXml.createDocument('', '', true);
    const node = doc.createElement('root');
    doc.appendChild(node);

    const serializedNode = domXml.serialize(node);
    assertTrue(/<root ?\/>/.test(serializedNode));

    const serializedDoc = domXml.serialize(doc);
    assertTrue(/(<\?xml version="1.0"\?>)?<root ?\/>/.test(serializedDoc));
  },

  /**
     @suppress {strictMissingProperties} suppression added to enable type
     checking
   */
  testBelowMaxDepthInIE() {
    if (userAgent.IE && !userAgent.isVersionOrHigher('9')) {
      // This value is only effective in IE8 and below
      domXml.MAX_ELEMENT_DEPTH = 5;
      const junk = '<a><b><c><d><e>Hello</e></d></c></b></a>';
      const doc = domXml.loadXml(junk);
      assertEquals(
          'Should not have caused a parse error', 0, Number(doc.parseError));
    }
  },

  /**
     @suppress {strictMissingProperties} suppression added to enable type
     checking
   */
  testAboveMaxDepthInIE() {
    if (userAgent.IE && !userAgent.isVersionOrHigher('9')) {
      // This value is only effective in IE8 and below
      domXml.MAX_ELEMENT_DEPTH = 4;
      const junk = '<a><b><c><d><e>Hello</e></d></c></b></a>';
      const doc = domXml.loadXml(junk);
      assertNotEquals(
          'Should have caused a parse error', 0, Number(doc.parseError));
    }
  },

  /**
     @suppress {strictMissingProperties} suppression added to enable type
     checking
   */
  testBelowMaxSizeInIE() {
    if (userAgent.IE && !userAgent.isVersionOrHigher('9')) {
      // This value is only effective in IE8 and below
      domXml.MAX_XML_SIZE_KB = 1;
      const junk = '<a>' + new Array(50).join('<b>junk</b>') + '</a>';
      const doc = domXml.loadXml(junk);
      assertEquals(
          'Should not have caused a parse error', 0, Number(doc.parseError));
    }
  },

  /**
     @suppress {strictMissingProperties} suppression added to enable type
     checking
   */
  testMaxSizeInIE() {
    if (userAgent.IE && !userAgent.isVersionOrHigher('9')) {
      // This value is only effective in IE8 and below
      domXml.MAX_XML_SIZE_KB = 1;
      const junk = '<a>' + new Array(1000).join('<b>junk</b>') + '</a>';
      const doc = domXml.loadXml(junk);
      assertNotEquals(
          'Should have caused a parse error', 0, Number(doc.parseError));
    }
  },

  testSelectSingleNodeNoActiveX() {
    if (userAgent.IE) {
      return;
    }

    const xml = domXml.loadXml('<a><b><c>d</c></b></a>');
    const node = xml.firstChild;
    const bNode = domXml.selectSingleNode(node, 'b');
    assertNotNull(bNode);
  },

  testSelectSingleNodeWithActiveX() {
    // Enable ActiveXObject so IE has xpath support.
    const xml = domXml.loadXml('<a><b><c>d</c></b></a>', true);
    const node = xml.firstChild;
    const bNode = domXml.selectSingleNode(node, 'b');
    assertNotNull(bNode);
  },

  testSelectNodesNoActiveX() {
    if (userAgent.IE) {
      return;
    }

    const xml = domXml.loadXml('<a><b><c>d</c></b><b>foo</b></a>');
    const node = xml.firstChild;
    const bNodes = domXml.selectNodes(node, 'b');
    assertNotNull(bNodes);
    assertEquals(2, bNodes.length);
  },

  testSelectNodesWithActiveX() {
    const xml = domXml.loadXml('<a><b><c>d</c></b><b>foo</b></a>', true);
    const node = xml.firstChild;
    const bNodes = domXml.selectNodes(node, 'b');
    assertNotNull(bNodes);
    assertEquals(2, bNodes.length);
  },

  testSetAttributes() {
    const xmlElement = domXml.createDocument().createElement('root');
    const domElement = dom.createElement(TagName.DIV);
    const attrs =
        {name: 'test3', title: 'A title', random: 'woop', cellpadding: '123'};

    domXml.setAttributes(xmlElement, attrs);
    domXml.setAttributes(domElement, attrs);

    assertEquals('test3', xmlElement.getAttribute('name'));
    assertEquals('test3', domElement.getAttribute('name'));

    assertEquals('A title', xmlElement.getAttribute('title'));
    assertEquals('A title', domElement.getAttribute('title'));

    assertEquals('woop', xmlElement.getAttribute('random'));
    assertEquals('woop', domElement.getAttribute('random'));

    assertEquals('123', xmlElement.getAttribute('cellpadding'));
    assertEquals('123', domElement.getAttribute('cellpadding'));
  },
});
