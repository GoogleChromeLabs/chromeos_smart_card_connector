/*
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Rhino code, released
 * May 6, 1999.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1997-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Nick Santos
 *   Google Inc.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 or later (the "GPL"), in which
 * case the provisions of the GPL are applicable instead of those above. If
 * you wish to allow use of your version of this file only under the terms of
 * the GPL and not to allow others to use your version of this file under the
 * MPL, indicate your decision by deleting the provisions above and replacing
 * them with the notice and other provisions required by the GPL. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * ***** END LICENSE BLOCK ***** */

package com.google.javascript.rhino.jstype;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Sets;
import com.google.javascript.rhino.jstype.Property.OwnedProperty;
import java.io.Serializable;
import java.util.HashSet;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.TreeMap;

/**
 * Representation for a collection of properties on an object.
 * @author nicksantos@google.com (Nick Santos)
 */
class PropertyMap implements Serializable {
  private static final long serialVersionUID = 1L;

  private static final PropertyMap EMPTY_MAP = new PropertyMap(
      ImmutableMap.<String, Property>of());

  // A place to get the inheritance structure.
  // Because the extended interfaces are resolved dynamically, this gets
  // messy :(. If type-resolution was more well-defined, we could
  // just reference primary parents and secondary parents directly.
  private ObjectType parentSource = null;

  // The map of our own properties.
  private final Map<String, Property> properties;

  PropertyMap() {
    this(new TreeMap<>());
  }

  private PropertyMap(Map<String, Property> underlyingMap) {
    this.properties = underlyingMap;
  }

  static PropertyMap immutableEmptyMap() {
    return EMPTY_MAP;
  }

  void setParentSource(ObjectType ownerType) {
    if (this != EMPTY_MAP) {
      this.parentSource = ownerType;
    }
  }

  /** Returns the direct parent of this property map. */
  PropertyMap getPrimaryParent() {
    if (parentSource == null) {
      return null;
    }
    ObjectType iProto = parentSource.getImplicitPrototype();
    return iProto == null ? null : iProto.getPropertyMap();
  }

  /**
   * Returns the secondary parents of this property map, for interfaces that need multiple
   * inheritance or for interfaces of abstract classes.
   */
  private Iterable<ObjectType> getSecondaryParentObjects() {
    if (parentSource == null) {
      return ImmutableList.of();
    }
    if (parentSource.getConstructor() != null && parentSource.getConstructor().isAbstract()) {
      return parentSource.getConstructor().getOwnImplementedInterfaces();
    }
    return parentSource.getCtorExtendedInterfaces();
  }

  OwnedProperty findTopMost(String name) {
    // Check primary parents which always has precendence over secondary.
    OwnedProperty found = null;
    for (PropertyMap map = this; map != null; map = map.getPrimaryParent()) {
      Property prop = map.properties.get(name);
      if (prop != null) {
        found = new OwnedProperty(map.parentSource, prop);
      }
    }
    if (found != null) {
      return found;
    }

    // Recurse into secondary parents. Note that there is no single top most definition with
    // interfaces so we simple return the first result.
    for (PropertyMap map = this; map != null; map = map.getPrimaryParent()) {
      for (ObjectType o : map.getSecondaryParentObjects()) {
        PropertyMap parent = o.getPropertyMap();
        if (parent != null) {
          OwnedProperty e = parent.findTopMost(name);
          if (e != null) {
            return e;
          }
        }
      }
    }

    return null;
  }

  OwnedProperty findClosest(String name) {
    // Check primary parents which always has precendence over secondary.
    for (PropertyMap map = this; map != null; map = map.getPrimaryParent()) {
      Property prop = map.properties.get(name);
      if (prop != null) {
        return new OwnedProperty(map.parentSource, prop);
      }
    }

    // Recurse into secondary parents.
    for (PropertyMap map = this; map != null; map = map.getPrimaryParent()) {
      for (ObjectType o : map.getSecondaryParentObjects()) {
        PropertyMap parent = o.getPropertyMap();
        if (parent != null) {
          OwnedProperty e = parent.findClosest(name);
          if (e != null) {
            return e;
          }
        }
      }
    }

    return null;
  }

  Property getOwnProperty(String propertyName) {
    return properties.get(propertyName);
  }

  int getPropertiesCount() {
    PropertyMap primaryParent = getPrimaryParent();
    if (primaryParent == null) {
      return this.properties.size();
    }
    Set<String> props = new HashSet<>();
    collectPropertyNames(props);
    return props.size();
  }

  Set<String> getOwnPropertyNames() {
    return properties.keySet();
  }

  void collectPropertyNames(Set<String> props) {
    Set<PropertyMap> identitySet = Sets.newIdentityHashSet();
    collectPropertyNamesHelper(props, identitySet);
  }

  // The interface inheritance chain can have cycles.
  // Use cache to avoid stack overflow.
  private void collectPropertyNamesHelper(
      Set<String> props, Set<PropertyMap> cache) {
    if (!cache.add(this)) {
      return;
    }
    props.addAll(properties.keySet());
    PropertyMap primaryParent = getPrimaryParent();
    if (primaryParent != null) {
      primaryParent.collectPropertyNamesHelper(props, cache);
    }
    for (ObjectType o : getSecondaryParentObjects()) {
      PropertyMap p = o.getPropertyMap();
      if (p != null) {
        p.collectPropertyNamesHelper(props, cache);
      }
    }
  }


  boolean removeProperty(String name) {
    return properties.remove(name) != null;
  }

  void putProperty(String name, Property newProp) {
    Property oldProp = properties.get(name);
    if (oldProp != null) {
      // This is to keep previously inferred JsDoc info, e.g., in a
      // replaceScript scenario.
      newProp.setJSDocInfo(oldProp.getJSDocInfo());
    }
    properties.put(name, newProp);
  }

  Iterable<Property> values() {
    return properties.values();
  }

  @Override
  public int hashCode() {
    // Calculate the hash just based on the property names, not their types.
    // Otherwise we can get into an infinite loop because the ObjectType hashCode
    // method calls this one.
    return Objects.hashCode(properties.keySet());
  }
}
