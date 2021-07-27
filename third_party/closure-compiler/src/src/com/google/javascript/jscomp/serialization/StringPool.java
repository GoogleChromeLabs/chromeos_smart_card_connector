/*
 * Copyright 2021 The Closure Compiler Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.javascript.jscomp.serialization;

import static com.google.common.base.Preconditions.checkNotNull;
import static com.google.common.base.Preconditions.checkState;
import static com.google.common.collect.ImmutableList.toImmutableList;

import com.google.common.collect.ImmutableList;
import com.google.errorprone.annotations.Immutable;
import java.util.LinkedHashMap;

/**
 * Aggregates strings into a {@link StringPool}
 *
 * <p>The zeroth offset in the string pool is always the empty string. This is validated inside
 * {@link TypedAstDeserializer}.
 *
 * <p>This implies default/unset/0-valued uuint32 StringPool pointers in protos are equivalent to
 * the empty string.
 */
@Immutable
public final class StringPool {

  private final int maxLength;
  private final ImmutableList<String> pool;

  static StringPool fromProto(StringPoolProto proto) {
    Wtf8.Decoder decoder = Wtf8.decoder(proto.getMaxLength());
    return new StringPool(
        proto.getMaxLength(),
        proto.getStringsList().stream().map(decoder::decode).collect(toImmutableList()));
  }

  public static StringPool empty() {
    return EMPTY;
  }

  private StringPool(int maxLength, ImmutableList<String> pool) {
    this.maxLength = maxLength;
    this.pool = pool;

    checkState(pool.get(0).isEmpty());
  }

  String get(int offset) {
    return this.pool.get(offset);
  }

  StringPoolProto toProto() {
    StringPoolProto.Builder builder = StringPoolProto.newBuilder().setMaxLength(this.maxLength);
    this.pool.stream().map(Wtf8::encodeToWtf8).forEachOrdered(builder::addStrings);
    return builder.build();
  }

  static Builder builder() {
    return new Builder();
  }

  static final class Builder {
    private int maxLength = 0;
    private final LinkedHashMap<String, Integer> pool = new LinkedHashMap<>();

    private Builder() {
      this.put("");
    }

    /** Inserts the given string into the string pool if not present and returns its index */
    int put(String string) {
      checkNotNull(string);

      if (string.length() > this.maxLength) {
        this.maxLength = string.length();
      }

      return this.pool.computeIfAbsent(string, (unused) -> this.pool.size());
    }

    Builder putAnd(String string) {
      this.put(string);
      return this;
    }

    StringPool build() {
      return new StringPool(this.maxLength, ImmutableList.copyOf(this.pool.keySet()));
    }
  }

  private static final StringPool EMPTY = new StringPool(0, ImmutableList.of(""));
}
