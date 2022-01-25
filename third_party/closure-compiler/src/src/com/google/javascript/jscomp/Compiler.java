/*
 * Copyright 2004 The Closure Compiler Authors.
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
package com.google.javascript.jscomp;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkNotNull;
import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.collect.ImmutableList.toImmutableList;
import static java.lang.Math.min;

import com.google.common.annotations.GwtIncompatible;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Joiner;
import com.google.common.base.Optional;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterables;
import com.google.common.collect.LinkedHashMultimap;
import com.google.common.collect.Multimap;
import com.google.common.collect.Sets;
import com.google.common.collect.Streams;
import com.google.debugging.sourcemap.SourceMapConsumerV3;
import com.google.debugging.sourcemap.proto.Mapping.OriginalMapping;
import com.google.javascript.jscomp.CompilerInput.ModuleType;
import com.google.javascript.jscomp.CompilerOptions.DevMode;
import com.google.javascript.jscomp.CompilerOptions.InstrumentOption;
import com.google.javascript.jscomp.JSChunkGraph.ChunkDependenceException;
import com.google.javascript.jscomp.JSChunkGraph.MissingChunkException;
import com.google.javascript.jscomp.NodeTraversal.AbstractPreOrderCallback;
import com.google.javascript.jscomp.SortingErrorManager.ErrorReportGenerator;
import com.google.javascript.jscomp.colors.ColorRegistry;
import com.google.javascript.jscomp.deps.BrowserModuleResolver;
import com.google.javascript.jscomp.deps.BrowserWithTransformedPrefixesModuleResolver;
import com.google.javascript.jscomp.deps.JsFileRegexParser;
import com.google.javascript.jscomp.deps.ModuleLoader;
import com.google.javascript.jscomp.deps.ModuleLoader.ModulePath;
import com.google.javascript.jscomp.deps.ModuleLoader.ModuleResolverFactory;
import com.google.javascript.jscomp.deps.ModuleLoader.PathResolver;
import com.google.javascript.jscomp.deps.NodeModuleResolver;
import com.google.javascript.jscomp.deps.SortedDependencies.MissingProvideException;
import com.google.javascript.jscomp.deps.WebpackModuleResolver;
import com.google.javascript.jscomp.diagnostic.LogFile;
import com.google.javascript.jscomp.instrumentation.CoverageInstrumentationPass;
import com.google.javascript.jscomp.instrumentation.CoverageInstrumentationPass.CoverageReach;
import com.google.javascript.jscomp.modules.ModuleMap;
import com.google.javascript.jscomp.modules.ModuleMetadataMap;
import com.google.javascript.jscomp.parsing.Config;
import com.google.javascript.jscomp.parsing.Config.JsDocParsing;
import com.google.javascript.jscomp.parsing.Config.LanguageMode;
import com.google.javascript.jscomp.parsing.Config.RunMode;
import com.google.javascript.jscomp.parsing.Config.StrictMode;
import com.google.javascript.jscomp.parsing.ParserRunner;
import com.google.javascript.jscomp.parsing.parser.FeatureSet;
import com.google.javascript.jscomp.parsing.parser.FeatureSet.Feature;
import com.google.javascript.jscomp.parsing.parser.trees.Comment;
import com.google.javascript.jscomp.parsing.parser.util.format.SimpleFormat;
import com.google.javascript.jscomp.resources.ResourceLoader;
import com.google.javascript.jscomp.serialization.ColorPool;
import com.google.javascript.jscomp.serialization.SerializeTypedAstPass;
import com.google.javascript.jscomp.serialization.TypedAstDeserializer;
import com.google.javascript.jscomp.type.ChainableReverseAbstractInterpreter;
import com.google.javascript.jscomp.type.ClosureReverseAbstractInterpreter;
import com.google.javascript.jscomp.type.ReverseAbstractInterpreter;
import com.google.javascript.jscomp.type.SemanticReverseAbstractInterpreter;
import com.google.javascript.rhino.ErrorReporter;
import com.google.javascript.rhino.IR;
import com.google.javascript.rhino.InputId;
import com.google.javascript.rhino.Node;
import com.google.javascript.rhino.StaticScope;
import com.google.javascript.rhino.jstype.JSTypeRegistry;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.Serializable;
import java.nio.file.Files;
import java.util.AbstractSet;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Supplier;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import javax.annotation.Nullable;

/**
 * Compiler (and the other classes in this package) does the following:
 *
 * <ul>
 *   <li>parses JS code
 *   <li>checks for undefined variables
 *   <li>performs optimizations such as constant folding and constants inlining
 *   <li>renames variables (to short names)
 *   <li>outputs compact JavaScript code
 * </ul>
 *
 * External variables are declared in 'externs' files. For instance, the file may include
 * definitions for global javascript/browser objects such as window, document.
 */
// TODO(tbreisacher): Rename Compiler to JsCompiler and remove this suppression.
@SuppressWarnings("JavaLangClash")
public class Compiler extends AbstractCompiler implements ErrorHandler, SourceFileMapping {
  static final DiagnosticType MODULE_DEPENDENCY_ERROR =
      DiagnosticType.error(
          "JSC_MODULE_DEPENDENCY_ERROR",
          "Bad dependency: {0} -> {1}. " + "Modules must be listed in dependency order.");

  static final DiagnosticType MISSING_ENTRY_ERROR =
      DiagnosticType.error(
          "JSC_MISSING_ENTRY_ERROR", "required entry point \"{0}\" never provided");

  static final DiagnosticType MISSING_MODULE_ERROR =
      DiagnosticType.error(
          "JSC_MISSING_MODULE_ERROR", "unknown module \"{0}\" specified in entry point spec");

  private CompilerOptions options = null;

  private PassConfig passes = null;

  // The externs inputs
  private final ArrayList<CompilerInput> externs = new ArrayList<>();

  // The source module graph, denoting dependencies between chunks.
  private JSChunkGraph moduleGraph;

  // The module loader for resolving paths into module URIs.
  private ModuleLoader moduleLoader;

  // Map of module names to module types - used for module rewriting
  private final Map<String, ModuleType> moduleTypesByName;

  // error manager to which error management is delegated
  private ErrorManager errorManager;

  // Warnings guard for filtering warnings.
  private WarningsGuard warningsGuard;

  // Compile-time injected libraries
  private final LinkedHashSet<String> injectedLibraries = new LinkedHashSet<>();

  // Node of the final injected library. Future libraries will be injected
  // after this node.
  private Node lastInjectedLibrary;

  // Parse tree root nodes
  private Node externsRoot;
  private Node jsRoot;
  private Node externAndJsRoot;

  // Used for debugging; to see the compiled code between passes
  private String lastJsSource = null;

  private FeatureSet featureSet;

  private final Map<InputId, CompilerInput> inputsById = new ConcurrentHashMap<>();

  private final Map<String, Node> scriptNodeByFilename = new ConcurrentHashMap<>();

  private ImmutableMap<String, String> inputPathByWebpackId;

  private LocaleData localeDataValueMap;

  private StaticScope transpilationNamespace;

  /**
   * Subclasses are responsible for loading sources that were not provided as explicit inputs to the
   * compiler. For example, looking up sources referenced within sourcemaps.
   */
  public static class ExternalSourceLoader {
    public SourceFile loadSource(String filename) {
      throw new RuntimeException("Cannot load without a valid loader.");
    }
  }

  // Original sources referenced by the source maps.
  private final ConcurrentHashMap<String, SourceFile> sourceMapOriginalSources =
      new ConcurrentHashMap<>();

  /** Configured {@link SourceMapInput}s, plus any source maps discovered in source files. */
  ConcurrentHashMap<String, SourceMapInput> inputSourceMaps = new ConcurrentHashMap<>();

  // Map from filenames to lists of all the comments in each file.
  private Map<String, List<Comment>> commentsPerFile = new ConcurrentHashMap<>();

  /** The source code map */
  private SourceMap sourceMap;

  /** The externs created from the exports. */
  private String externExports = null;

  private UniqueIdSupplier uniqueIdSupplier = new UniqueIdSupplier();

  /** Ids for function inlining so that each declared name remains unique. */
  private int uniqueNameId = 0;

  /** Whether to assume there are references to the RegExp Global object properties. */
  private boolean hasRegExpGlobalReferences = true;

  private boolean runJ2clPasses = false;

  /** Detects Google-specific coding conventions. */
  CodingConvention defaultCodingConvention = new ClosureCodingConvention();

  private JSTypeRegistry typeRegistry;
  private ColorRegistry colorRegistry;
  private volatile Config parserConfig = null;
  private volatile Config externsParserConfig = null;

  private ReverseAbstractInterpreter abstractInterpreter;
  private TypeValidator typeValidator;
  // The compiler can ask phaseOptimizer for things like which pass is currently
  // running, or which functions have been changed by optimizations
  private PhaseOptimizer phaseOptimizer = null;

  public PerformanceTracker tracker;

  /** Runtime-library files deserailized from a TypedAST JAR resource; indexed by filename */
  private ImmutableMap<String, Supplier<Node>> runtimeLibraryTypedAsts;

  // Types that have been forward declared
  private Set<String> forwardDeclaredTypes = new HashSet<>();

  private boolean typeCheckingHasRun = false;

  // This error reporter gets the messages from the current Rhino parser or TypeRegistry.
  private final ErrorReporter oldErrorReporter = RhinoErrorReporter.forOldRhino(this);

  /** Error strings used for reporting JSErrors */
  public static final DiagnosticType OPTIMIZE_LOOP_ERROR =
      DiagnosticType.error(
          "JSC_OPTIMIZE_LOOP_ERROR", "Exceeded max number of optimization iterations: {0}");

  public static final DiagnosticType MOTION_ITERATIONS_ERROR =
      DiagnosticType.error(
          "JSC_MOTION_ITERATIONS_ERROR", "Exceeded max number of code motion iterations: {0}");

  private final CompilerExecutor compilerExecutor = createCompilerExecutor();

  /**
   * Logger for the whole com.google.javascript.jscomp domain - setting configuration for this
   * logger affects all loggers in other classes within the compiler.
   */
  public static final Logger logger = Logger.getLogger("com.google.javascript.jscomp");

  private final PrintStream outStream;

  private volatile double progress = 0.0;
  private String lastPassName;

  private ImmutableSet<String> externProperties = null;
  private AccessorSummary accessorSummary = null;

  private static final Joiner pathJoiner = Joiner.on(Platform.getFileSeperator());

  // Starts at 0, increases as "interesting" things happen.
  // Nothing happens at time START_TIME, the first pass starts at time 1.
  // The correctness of scope-change tracking relies on Node/getIntProp
  // returning 0 if the custom attribute on a node hasn't been set.
  private int changeStamp = 1;

  private final Timeline<Node> changeTimeline = new Timeline<>();
  private final Timeline<Node> deleteTimeline = new Timeline<>();

  /**
   * When mapping symbols from a source map, we must repeatedly combine the path of the original
   * file with the path from the source map to compute the SourceFile of the underlying code. When
   * iterating through adjacent symbols this computation ends up computing the same thing.
   *
   * <p>This object tracks the last source map we resolved to reduce that work. It caches: for a
   * given originalPath+sourceMapPath, what sourceFile did we find?
   */
  private static class ResolvedSourceMap {
    public String originalPath;
    public String sourceMapPath;
    public String relativePath;
  }

  /**
   * There is exactly one cached resolvedSourceMap, which you can think of as a cache of size one.
   */
  private final ResolvedSourceMap resolvedSourceMap = new ResolvedSourceMap();

  /** Creates a Compiler that reports errors and warnings to its logger. */
  public Compiler() {
    this((PrintStream) null);
  }

  /** Creates a Compiler that reports errors and warnings to an output stream. */
  public Compiler(PrintStream outStream) {
    addChangeHandler(recentChange);
    this.outStream = outStream;
    this.moduleTypesByName = new HashMap<>();
  }

  /** Creates a Compiler that uses a custom error manager. */
  public Compiler(ErrorManager errorManager) {
    this();
    setErrorManager(errorManager);
  }

  /**
   * Sets the error manager.
   *
   * @param errorManager the error manager, it cannot be {@code null}
   */
  public void setErrorManager(ErrorManager errorManager) {
    checkNotNull(errorManager, "the error manager cannot be null");
    this.errorManager = new ThreadSafeDelegatingErrorManager(errorManager);
  }

  /** Creates a message formatter instance corresponding to the value of {@link CompilerOptions}. */
  private MessageFormatter createMessageFormatter() {
    boolean colorize = options.shouldColorizeErrorOutput();
    return options.errorFormat.toFormatter(this, colorize);
  }

  /**
   * Initializes the compiler options. It's called as part of a normal compile() job. Public for the
   * callers that are not doing a normal compile() job.
   */
  public void initOptions(CompilerOptions options) {
    this.options = options;
    this.setFeatureSet(options.getLanguageIn().toFeatureSet());
    if (errorManager == null) {
      if (this.outStream == null) {
        setErrorManager(new LoggerErrorManager(createMessageFormatter(), logger));
      } else {
        ImmutableSet.Builder<ErrorReportGenerator> builder = ImmutableSet.builder();
        builder.add(
            new PrintStreamErrorReportGenerator(
                createMessageFormatter(), this.outStream, options.summaryDetailLevel));
        builder.addAll(options.getExtraReportGenerators());
        setErrorManager(new SortingErrorManager(builder.build()));
      }
    }

    moduleLoader = ModuleLoader.EMPTY;

    reconcileOptionsWithGuards();

    // TODO(johnlenz): generally, the compiler should not be changing the options object
    // provided by the user.  This should be handled a different way.

    // Turn off type-based optimizations when type checking is off
    if (!options.isTypecheckingEnabled()) {
      options.setUseTypesForLocalOptimization(false);
      options.setUseTypesForOptimization(false);
    }

    if (options.assumeForwardDeclaredForMissingTypes) {
      this.forwardDeclaredTypes =
          new AbstractSet<String>() {
            @Override
            public boolean contains(Object o) {
              return true; // Report all types as forward declared types.
            }

            @Override
            public boolean add(String e) {
              return false;
            }

            @Override
            public Iterator<String> iterator() {
              return Collections.emptyIterator();
            }

            @Override
            public int size() {
              return 0;
            }
          };
    }

    initWarningsGuard(options.getWarningsGuard());
    if (this.isDebugLoggingEnabled()) {
      // If debug logs are requested, then we'll always log the configuration for convenience.
      options.setPrintConfig(true);
    }

    if (options.isCheckingMissingOverrideTypes()) {
      // this allows us to generate syntactically better replacement JSDoc that preserves existing
      // description.
      options.setParseJsDocDocumentation(JsDocParsing.INCLUDE_ALL_COMMENTS);
    }
  }

  public void printConfig() {
    if (this.isDebugLoggingEnabled()) {
      // Log to separate files for convenience.
      logToFile("externs.log", externs::toString);
      // To get a pretty-printed JSON module graph, change the string generation expression to
      //
      // new GsonBuilder().setPrettyPrinting().create().toJson(moduleGraph.toJson())
      //
      // TODO(bradfordcsmith): Come up with a JSON-printing version that will work when this code is
      // compiled with J2CL, so we can permanently improve this.
      logToFile("inputs.json", () -> Iterables.toString(moduleGraph.getAllInputs()));
      logToFile("options.log", () -> options.toString());
      logToFile("warningsGuard.log", () -> warningsGuard.toString());
    } else {
      // Debug logging is not enabled, so use stderr
      final PrintStream err = System.err;
      err.println("==== Externs ====");
      err.println(externs);
      err.println("==== Inputs ====");
      // To get a pretty-printed JSON module graph, change this line to
      //
      // err.println(
      //     new GsonBuilder().setPrettyPrinting().create().toJson(moduleGraph.toJson()));
      //
      // TODO(bradfordcsmith): Come up with a JSON-printing version that will work when this code is
      // compiled with J2CL, so we can permanently improve this.
      err.println(Iterables.toString(moduleGraph.getAllInputs()));
      err.println("==== CompilerOptions ====");
      err.println(options);
      err.println("==== WarningsGuard ====");
      err.println(warningsGuard);
    }
  }

  private void logToFile(String logFileName, Supplier<String> logStringSupplier) {
    try (LogFile log = this.createOrReopenLog(this.getClass(), logFileName)) {
      log.log(logStringSupplier);
    }
  }

  private void initWarningsGuard(WarningsGuard warningsGuard) {
    ImmutableList.Builder<WarningsGuard> guards = ImmutableList.builder();
    guards
        .add(new J2clSuppressWarningsGuard())
        .add(new SuppressDocWarningsGuard(this, DiagnosticGroups.getRegisteredGroups()))
        .add(warningsGuard);
    if (this.options != null && this.options.shouldSkipUnsupportedPasses()) {
      guards.add(
          new DiagnosticGroupWarningsGuard(
              DiagnosticGroups.FEATURES_NOT_SUPPORTED_BY_PASS, CheckLevel.WARNING));
    }

    this.warningsGuard = new ComposeWarningsGuard(guards.build());
  }

  /** When the CompilerOptions and its WarningsGuard overlap, reconcile any discrepancies. */
  protected void reconcileOptionsWithGuards() {
    // DiagnosticGroups override the plain checkTypes option.
    if (options.enables(DiagnosticGroups.CHECK_TYPES)) {
      options.checkTypes = true;
    } else if (options.disables(DiagnosticGroups.CHECK_TYPES)) {
      options.checkTypes = false;
    } else if (!options.checkTypes) {
      // If DiagnosticGroups did not override the plain checkTypes
      // option, and checkTypes is disabled, then turn off the
      // parser type warnings.
      options.setWarningLevel(
          DiagnosticGroup.forType(RhinoErrorReporter.TYPE_PARSE_ERROR), CheckLevel.OFF);
    }

    if (!options.checkTypes) {
      options.setWarningLevel(DiagnosticGroups.BOUNDED_GENERICS, CheckLevel.OFF);
    }

    // All passes must run the variable check. This synthesizes
    // variables later so that the compiler doesn't crash. It also
    // checks the externs file for validity. If you don't want to warn
    // about missing variable declarations, we shut that specific
    // error off.
    if (!options.checkSymbols && !options.enables(DiagnosticGroups.CHECK_VARIABLES)) {
      options.setWarningLevel(DiagnosticGroups.CHECK_VARIABLES, CheckLevel.OFF);
    }

    // If we're in transpile-only mode, we don't need to do checks for suspicious var usages.
    // Since we still have to run VariableReferenceCheck before transpilation to check block-scoped
    // variables, though, we disable the unnecessary warnings it produces relating to vars here.
    if (options.skipNonTranspilationPasses && !options.enables(DiagnosticGroups.CHECK_VARIABLES)) {
      options.setWarningLevel(DiagnosticGroups.CHECK_VARIABLES, CheckLevel.OFF);
    }

    // If we're in transpile-only mode, we don't need to check for missing requires unless the user
    // explicitly enables missing-provide checks.
    if (options.skipNonTranspilationPasses && !options.enables(DiagnosticGroups.MISSING_PROVIDE)) {
      options.setWarningLevel(DiagnosticGroups.MISSING_PROVIDE, CheckLevel.OFF);
    }
  }

  private ImmutableMap<SourceFile, Supplier<Node>> typedAstFilesystem;

  @Nullable
  @Override
  Supplier<Node> getTypedAstDeserializer(SourceFile file) {
    if (this.typedAstFilesystem == null) {
      return null;
    }

    Supplier<Node> ast = this.typedAstFilesystem.get(file);
    checkState(
        ast != null || file.equals(SYNTHETIC_EXTERNS_FILE),
        "TypedAST filesystem initialized, but missing requested file: %s",
        file);
    return ast;
  }

  /**
   * Initializes a compiler that will use pre-compiled TypedAst files instead of reading the source
   * from disk
   */
  @GwtIncompatible
  public final void initWithTypedAstFilesystem(
      List<SourceFile> externs,
      List<SourceFile> sources,
      CompilerOptions options,
      InputStream typedAstListStream) {

    ImmutableList<SourceFile> files =
        ImmutableList.<SourceFile>builder().addAll(externs).addAll(sources).build();

    this.initTypedAstFilesystem(files, typedAstListStream);
    this.init(externs, sources, options);
  }

  /**
   * Initializes a compiler that will use pre-compiled TypedAst files instead of reading the source
   * from disk
   */
  @GwtIncompatible
  public void initModulesWithTypedAstFilesystem(
      List<SourceFile> externs,
      List<JSChunk> modules,
      CompilerOptions options,
      InputStream typedAstListStream) {

    ImmutableList.Builder<SourceFile> filesBuilder = ImmutableList.builder();
    filesBuilder.addAll(externs);
    for (JSChunk chunk : modules) {
      for (CompilerInput input : chunk.getInputs()) {
        filesBuilder.add(input.getSourceFile());
      }
    }
    ImmutableList<SourceFile> files = filesBuilder.build();

    this.initTypedAstFilesystem(files, typedAstListStream);
    this.initModules(externs, modules, options);
  }

  @GwtIncompatible
  private void initTypedAstFilesystem(
      ImmutableList<SourceFile> existingSourceFiles, InputStream typedAstListStream) {
    checkState(this.typedAstFilesystem == null);

    this.setLifeCycleStage(LifeCycleStage.COLORS_AND_SIMPLIFIED_JSDOC);
    TypedAstDeserializer.DeserializedAst astData =
        TypedAstDeserializer.deserializeFullAst(
            this,
            SYNTHETIC_EXTERNS_FILE,
            existingSourceFiles,
            typedAstListStream,
            // TODO(b/183734515): consider setting 'includeTypeInformation' to false when no type-
            // based optimziation flags are enabled, as a performance optimization.
            /* includeTypeInformation= */ true);

    this.typedAstFilesystem = astData.getFilesystem();
    this.externProperties = astData.getExternProperties();
    this.colorRegistry = astData.getColorRegistry().get();
  }

  @Override
  @GwtIncompatible
  public void initRuntimeLibraryTypedAsts(Optional<ColorPool.Builder> colorPoolBuilder) {
    checkState(this.runtimeLibraryTypedAsts == null);

    String path =
        String.join(
            "",
            "/runtime_libs.typedast");

    TypedAstDeserializer.DeserializedAst astData =
        TypedAstDeserializer.deserializeRuntimeLibraries(
            this,
            SYNTHETIC_EXTERNS_FILE,
            colorPoolBuilder,
            Compiler.class.getResourceAsStream(path));

    // Re-index the runtime libraries by file name rather than SourceFile object
    LinkedHashMap<String, Supplier<Node>> runtimeLibraryTypedAsts = new LinkedHashMap<>();
    for (Map.Entry<SourceFile, Supplier<Node>> library : astData.getFilesystem().entrySet()) {
      runtimeLibraryTypedAsts.computeIfAbsent(
          library.getKey().getName(), (f) -> library.getValue());
    }

    this.runtimeLibraryTypedAsts = ImmutableMap.copyOf(runtimeLibraryTypedAsts);
  }

  /** Initializes the instance state needed for a compile job. */
  public final void init(
      List<SourceFile> externs, List<SourceFile> sources, CompilerOptions options) {
    JSChunk module = new JSChunk(JSChunk.STRONG_CHUNK_NAME);
    for (SourceFile source : sources) {
      module.add(new CompilerInput(source, /* isExtern */ false));
    }

    List<JSChunk> modules = new ArrayList<>(1);
    modules.add(module);
    initModules(externs, modules, options);
    addFilesToSourceMap(sources);
  }

  /** Initializes the instance state needed for a compile job if the sources are in modules. */
  public void initModules(
      List<SourceFile> externs, List<JSChunk> modules, CompilerOptions options) {
    initOptions(options);

    checkFirstModule(modules);

    this.externs.clear();
    for (SourceFile file : externs) {
      this.externs.add(new CompilerInput(file, /* isExtern */ true));
    }

    // Generate the module graph, and report any errors in the module specification as errors.
    try {
      this.moduleGraph = new JSChunkGraph(modules);
    } catch (ChunkDependenceException e) {
      // problems with the module format.  Report as an error.  The
      // message gives all details.
      report(
          JSError.make(
              MODULE_DEPENDENCY_ERROR, e.getChunk().getName(), e.getDependentChunk().getName()));
      return;
    }

    // Creating the module graph can move weak source around, and end up with empty modules.
    fillEmptyModules(getModules());

    this.commentsPerFile = new ConcurrentHashMap<>(moduleGraph.getInputCount());
    initBasedOnOptions();

    initInputsByIdMap();

    initAST();

    // If debug logging is enabled, write out the module / chunk graph in GraphViz format.
    // This graph is often too big to reasonably render.
    // Using gvpr(1) is recommended to extract the parts of the graph that are of interest.
    // `dot -Tpng graph_file.dot > graph_file.png` will render an image.
    try (LogFile moduleGraphLog = createOrReopenLog(this.getClass(), "chunk_graph.dot")) {
      moduleGraphLog.log(DotFormatter.toDot(moduleGraph.toGraphvizGraph()));
    }
  }

  /** Do any initialization that is dependent on the compiler options. */
  public void initBasedOnOptions() {
    inputSourceMaps.putAll(options.inputSourceMaps);
    // Create the source map if necessary.
    if (options.sourceMapOutputPath != null) {
      sourceMap = options.sourceMapFormat.getInstance();
      sourceMap.setPrefixMappings(options.sourceMapLocationMappings);
      if (options.applyInputSourceMaps) {
        sourceMap.setSourceFileMapping(this);
        if (options.sourceMapIncludeSourcesContent) {
          for (SourceMapInput inputSourceMap : inputSourceMaps.values()) {
            addSourceMapSourceFiles(inputSourceMap);
          }
        }
      }
    }
  }

  private static final DiagnosticType EMPTY_MODULE_LIST_ERROR =
      DiagnosticType.error("JSC_EMPTY_MODULE_LIST_ERROR", "At least one module must be provided");

  private static final DiagnosticType EMPTY_ROOT_MODULE_ERROR =
      DiagnosticType.error(
          "JSC_EMPTY_ROOT_MODULE_ERROR",
          "Root module ''{0}'' must contain at least one source code input");

  /**
   * Verifies that at least one module has been provided and that the first one has at least one
   * source code input.
   */
  private void checkFirstModule(List<JSChunk> modules) {
    if (modules.isEmpty()) {
      report(JSError.make(EMPTY_MODULE_LIST_ERROR));
    } else if (modules.get(0).getInputs().isEmpty() && modules.size() > 1) {
      // The root module may only be empty if there is exactly 1 module.
      report(JSError.make(EMPTY_ROOT_MODULE_ERROR, modules.get(0).getName()));
    }
  }

  /** Creates an OS specific path string from parts */
  public static String joinPathParts(String... pathParts) {
    return pathJoiner.join(pathParts);
  }

  /** Fill any empty modules with a place holder file. It makes any cross module motion easier. */
  private void fillEmptyModules(Iterable<JSChunk> modules) {
    for (JSChunk module : modules) {
      if (!module.getName().equals(JSChunk.WEAK_CHUNK_NAME) && module.getInputs().isEmpty()) {
        CompilerInput input =
            new CompilerInput(SourceFile.fromCode(createFillFileName(module.getName()), ""));
        input.setCompiler(this);
        module.add(input);
      }
    }
  }

  /**
   * Rebuilds the internal input map by iterating over all modules. This is necessary if inputs have
   * been added to or removed from a module after an {@link #init} or {@link #initModules} call.
   */
  public void rebuildInputsFromModules() {
    initInputsByIdMap();
  }

  static final DiagnosticType DUPLICATE_INPUT =
      DiagnosticType.error("JSC_DUPLICATE_INPUT", "Duplicate input: {0}");
  static final DiagnosticType DUPLICATE_EXTERN_INPUT =
      DiagnosticType.error("JSC_DUPLICATE_EXTERN_INPUT", "Duplicate extern input: {0}");

  /** Creates a map to make looking up an input by name fast. Also checks for duplicate inputs. */
  void initInputsByIdMap() {
    inputsById.clear();
    for (CompilerInput input : externs) {
      CompilerInput previous = putCompilerInput(input);
      if (previous != null) {
        report(JSError.make(DUPLICATE_EXTERN_INPUT, input.getName()));
      }
    }
    boolean hasZone = false;
    for (CompilerInput input : moduleGraph.getAllInputs()) {
      if (input.getName().endsWith("packages/zone.js/lib/zone.closure.js")) {
        hasZone = true;
      }
      CompilerInput previous = putCompilerInput(input);
      if (previous != null) {
        report(JSError.make(DUPLICATE_INPUT, input.getName()));
      }
    }
    if (hasZone
        && !options.allowsZoneJsWithAsyncFunctionsInOutput()
        && options.getOutputFeatureSet().contains(FeatureSet.Feature.ASYNC_FUNCTIONS)) {
      throw new UnsupportedOperationException(
          "ZoneJS is incompatible with language level ES2017 or higher (See go/ngissue/31730)\n"
              + "Please set `--language_out=ECMASCRIPT_2016` (or older) in your flags.");
    }
  }

  /** Sets up the skeleton of the AST (the externs and root). */
  private void initAST() {
    jsRoot = IR.root();
    externsRoot = IR.root();
    externAndJsRoot = IR.root(externsRoot, jsRoot);

    if (this.typedAstFilesystem != null) {
      this.getSynthesizedExternsInput(); // Force lazy creation.
    }
  }

  /** Compiles a single source file and a single externs file. */
  public Result compile(SourceFile extern, SourceFile input, CompilerOptions options) {
    return compile(ImmutableList.of(extern), ImmutableList.of(input), options);
  }

  /**
   * Compiles a list of inputs.
   *
   * <p>This is a convenience method to wrap up all the work of compilation, including generating
   * the error and warning report.
   *
   * <p>NOTE: All methods called here must be public, because client code must be able to replicate
   * and customize this.
   */
  public Result compile(
      List<SourceFile> externs, List<SourceFile> inputs, CompilerOptions options) {
    // The compile method should only be called once.
    checkState(jsRoot == null);

    try {
      init(externs, inputs, options);
      if (options.printConfig) {
        printConfig();
      }
      if (!hasErrors()) {
        parseForCompilation();
      }
      if (!hasErrors()) {
        if (options.getInstrumentForCoverageOnly()) {
          // TODO(bradfordcsmith): The option to instrument for coverage only should belong to the
          //     runner, not the compiler.
          instrumentForCoverage();
        } else {
          stage1Passes();
          if (!hasErrors()) {
            stage2Passes();
            if (!hasErrors()) {
              stage3Passes();
            }
          }
        }
        performPostCompilationTasks();
      }
    } finally {
      generateReport();
    }
    return getResult();
  }

  /**
   * Generates a report of all warnings and errors found during compilation to stderr.
   *
   * <p>Client code must call this method explicitly if it doesn't use one of the convenience
   * methods that do so automatically.
   *
   * <p>Always call this method, even if the compiler throws an exception. The report will include
   * information about the exception.
   */
  public void generateReport() {
    Tracer t = newTracer("generateReport");
    errorManager.generateReport();
    stopTracer(t, "generateReport");
  }

  /**
   * Compiles a list of modules.
   *
   * <p>This is a convenience method to wrap up all the work of compilation, including generating
   * the error and warning report.
   *
   * <p>NOTE: All methods called here must be public, because client code must be able to replicate
   * and customize this.
   */
  public Result compileModules(
      List<SourceFile> externs, List<JSChunk> modules, CompilerOptions options) {
    // The compile method should only be called once.
    checkState(jsRoot == null);

    try {
      initModules(externs, modules, options);
      if (options.printConfig) {
        printConfig();
      }
      if (!hasErrors()) {
        parseForCompilation();
      }
      if (!hasErrors()) {
        // TODO(bradfordcsmith): The option to instrument for coverage only should belong to the
        //     runner, not the compiler.
        if (options.getInstrumentForCoverageOnly()) {
          instrumentForCoverage();
        } else {
          stage1Passes();
          if (!hasErrors()) {
            stage2Passes();
            if (!hasErrors()) {
              stage3Passes();
            }
          }
        }
        performPostCompilationTasks();
      }
    } finally {
      generateReport();
    }
    return getResult();
  }

  /**
   * Perform compiler passes for stage 1 of compilation.
   *
   * <p>Stage 1 consists primarily of error and type checking passes.
   *
   * <p>{@code parseForCompilation()} must be called before this method is called.
   *
   * <p>The caller is responsible for also calling {@code generateReport()} to generate a report of
   * warnings and errors to stderr. See the invocation in {@link #compile} for a good example.
   */
  public void stage1Passes() {
    checkState(moduleGraph != null, "No inputs. Did you call init() or initModules()?");
    checkState(!hasErrors());
    checkState(!options.getInstrumentForCoverageOnly());
    runInCompilerThread(
        () -> {
          performChecks();
          return null;
        });
  }

  /**
   * Perform compiler passes for stage 2 of compilation.
   *
   * <p>Stage 2 consists primarily of transpilation and optimization passes.
   *
   * <p>{@code stage1Passes()} must be called before this method is called.
   *
   * <p>The caller is responsible for also calling {@code generateReport()} to generate a report of
   * warnings and errors to stderr. See the invocation in {@link #compile} for a good example.
   */
  public void stage2Passes() {
    checkState(moduleGraph != null, "No inputs. Did you call init() or initModules()?");
    checkState(!hasErrors());
    checkState(!options.getInstrumentForCoverageOnly());
    JSChunk weakModule = moduleGraph.getChunkByName(JSChunk.WEAK_CHUNK_NAME);
    if (weakModule != null) {
      for (CompilerInput i : moduleGraph.getAllInputs()) {
        if (i.getSourceFile().isWeak()) {
          checkState(
              i.getChunk() == weakModule, "Expected all weak files to be in the weak module.");
        }
      }
    }
    runInCompilerThread(
        () -> {
          if (options.shouldOptimize()) {
            performTranspilationAndOptimizations();
          }
          return null;
        });
  }

  /**
   * Perform compiler passes for stage 3 of compilation.
   *
   * <p>Stage 3 consists primarily of localization passes.
   *
   * <p>{@code stage2Passes()} must be called before this method is called.
   *
   * <p>The caller is responsible for also calling {@code generateReport()} to generate a report of
   * warnings and errors to stderr. See the invocation in {@link #compile} for a good example.
   */
  public void stage3Passes() {
    checkState(moduleGraph != null, "No inputs. Did you call init() or initModules()?");
    checkState(!hasErrors());
    checkState(!options.getInstrumentForCoverageOnly());
    runInCompilerThread(
        () -> {
          if (options.shouldOptimize()) {
            performFinalizations();
          }
          return null;
        });
  }

  /** Disable threads. This is for clients that run on AppEngine and don't have threads. */
  public void disableThreads() {
    compilerExecutor.disableThreads();
  }

  /**
   * Sets the timeout when Compiler is run in a thread
   *
   * @param timeout seconds to wait before timeout
   */
  public void setTimeout(int timeout) {
    compilerExecutor.setTimeout(timeout);
  }

  /**
   * The primary purpose of this method is to run the provided code with a larger than standard
   * stack.
   */
  <T> T runInCompilerThread(Callable<T> callable) {
    return compilerExecutor.runInCompilerThread(
        callable, options != null && options.getTracerMode().isOn());
  }

  private void performChecks() {
    if (options.skipNonTranspilationPasses) {
      // i.e. whitespace-only mode, which will not work with goog.module without:
      whitespaceOnlyPasses();
      if (options.needsTranspilationFrom(options.getLanguageIn().toFeatureSet())) {
        transpileAndDontCheck();
      }
    } else {
      check();
    }
  }

  /**
   * Performs all the bookkeeping required at the end of a compilation.
   *
   * <p>This method must be called if the compilation makes it as far as doing checks.
   *
   * <p>DON'T call it if the compiler threw an exception.
   *
   * <p>DO call it even when {@code hasErrors()} returns true.
   */
  public void performPostCompilationTasks() {
    runInCompilerThread(
        () -> {
          performPostCompilationTasksInternal();
          return null;
        });
  }

  /** Performs all the bookkeeping required at the end of a compilation. */
  private void performPostCompilationTasksInternal() {
    if (options.devMode == DevMode.START_AND_END) {
      runValidityCheck();
    }
    setProgress(1.0, "recordFunctionInformation");

    if (tracker != null) {
      if (options.getTracerOutput() == null) {
        tracker.outputTracerReport(this.outStream);
      } else {
        try (PrintStream out = new PrintStream(Files.newOutputStream(options.getTracerOutput()))) {
          tracker.outputTracerReport(out);
        } catch (Exception e) {
          throw new RuntimeException(e);
        }
      }
    }
  }

  /**
   * Instrument code for coverage.
   *
   * <p>{@code parseForCompilation()} must be called before this method is called.
   *
   * <p>The caller is responsible for also calling {@code generateReport()} to generate a report of
   * warnings and errors to stderr. See the invocation in {@link #compile} for a good example.
   *
   * <p>This method is mutually exclusive with stage1Passes() and stage2Passes(). Either call those
   * two methods or this one, but not both.
   */
  public void instrumentForCoverage() {
    checkState(moduleGraph != null, "No inputs. Did you call init() or initModules()?");
    checkState(!hasErrors());
    runInCompilerThread(
        () -> {
          checkState(options.getInstrumentForCoverageOnly());
          checkState(!hasErrors());
          if (options.getInstrumentForCoverageOption() != InstrumentOption.NONE) {
            instrumentForCoverageInternal(options.getInstrumentForCoverageOption());
          }
          return null;
        });
  }

  private void instrumentForCoverageInternal(InstrumentOption instrumentForCoverageOption) {
    Tracer tracer = newTracer("instrumentationPass");
    process(
        new CoverageInstrumentationPass(
            this,
            CoverageReach.ALL,
            instrumentForCoverageOption,
            options.getProductionInstrumentationArrayName()));
    stopTracer(tracer, "instrumentationPass");
  }

  /**
   * Parses input files in preparation for compilation.
   *
   * <p>Either {@code init()} or {@code initModules()} must be called first to set up the input
   * files to be read.
   *
   * <p>TODO(bradfordcsmith): Rename this to parse()
   */
  public void parseForCompilation() {
    runInCompilerThread(
        () -> {
          parseForCompilationInternal();
          return null;
        });
  }

  /**
   * Parses input files in preparation for compilation.
   *
   * <p>Either {@code init()} or {@code initModules()} must be called first to set up the input
   * files to be read.
   *
   * <p>TODO(bradfordcsmith): Rename this to parse()
   */
  private void parseForCompilationInternal() {
    setProgress(0.0, null);
    CompilerOptionsPreprocessor.preprocess(options);
    maybeSetTracker();
    parseInputs();
    // Guesstimate.
    setProgress(0.15, "parse");
  }

  /**
   * Parses input files without doing progress tracking that is part of a full compile.
   *
   * <p>Either {@code init()} or {@code initModules()} must be called first to set up the input
   * files to be read.
   *
   * <p>TODO(bradfordcsmith): Rename this to parseIndependentOfCompilation() or similar.
   */
  public void parse() {
    parseInputs();
  }

  public Node parse(SourceFile file) {
    initCompilerOptionsIfTesting();
    logger.finest("Parsing: " + file.getName());
    return new JsAst(file).getAstRoot(this);
  }

  PassConfig getPassConfig() {
    if (passes == null) {
      passes = createPassConfigInternal();
    }
    return passes;
  }

  /** Create the passes object. Clients should use setPassConfig instead of overriding this. */
  protected PassConfig createPassConfigInternal() {
    return new DefaultPassConfig(options);
  }

  /**
   * @param passes The PassConfig to use with this Compiler.
   * @throws NullPointerException if passes is null
   * @throws IllegalStateException if this.passes has already been assigned
   */
  public void setPassConfig(PassConfig passes) {
    // Important to check for null because if setPassConfig(null) is
    // called before this.passes is set, getPassConfig() will create a
    // new PassConfig object and use that, which is probably not what
    // the client wanted since they probably meant to use their
    // own PassConfig object.
    checkNotNull(passes);
    checkState(this.passes == null, "setPassConfig was already called");
    this.passes = passes;
  }

  public void whitespaceOnlyPasses() {
    runCustomPasses(CustomPassExecutionTime.BEFORE_CHECKS);

    Tracer t = newTracer("runWhitespaceOnlyPasses");
    try {
      for (PassFactory pf : getPassConfig().getWhitespaceOnlyPasses()) {
        pf.create(this).process(externsRoot, jsRoot);
      }
    } finally {
      stopTracer(t, "runWhitespaceOnlyPasses");
    }
  }

  public void transpileAndDontCheck() {
    Tracer t = newTracer("runTranspileOnlyPasses");
    try {
      for (PassFactory pf : getPassConfig().getTranspileOnlyPasses()) {
        if (hasErrors()) {
          return;
        }
        pf.create(this).process(externsRoot, jsRoot);
      }
    } finally {
      stopTracer(t, "runTranspileOnlyPasses");
    }
  }

  private PhaseOptimizer createPhaseOptimizer() {
    PhaseOptimizer phaseOptimizer = new PhaseOptimizer(this, tracker);
    if (options.devMode == DevMode.EVERY_PASS) {
      phaseOptimizer.setValidityCheck(validityCheck);
    }
    if (options.getCheckDeterminism()) {
      phaseOptimizer.setPrintAstHashcodes(true);
    }
    return phaseOptimizer;
  }

  void check() {
    runCustomPasses(CustomPassExecutionTime.BEFORE_CHECKS);

    // We are currently only interested in check-passes for progress reporting
    // as it is used for IDEs, that's why the maximum progress is set to 1.0.
    phaseOptimizer =
        createPhaseOptimizer().withProgress(new PhaseOptimizer.ProgressRange(getProgress(), 1.0));
    phaseOptimizer.consume(getPassConfig().getChecks());
    phaseOptimizer.process(externsRoot, jsRoot);
    if (hasErrors()) {
      return;
    }

    runCustomPasses(CustomPassExecutionTime.BEFORE_OPTIMIZATIONS);
    phaseOptimizer = null;
  }

  @Override
  void setExternExports(String externExports) {
    this.externExports = externExports;
  }

  @Override
  void process(CompilerPass p) {
    p.process(externsRoot, jsRoot);
  }

  private final PassFactory validityCheck =
      PassFactory.builder()
          .setName("validityCheck")
          .setRunInFixedPointLoop(true)
          .setInternalFactory(ValidityCheck::new)
          .setFeatureSetForChecks()
          .build();

  private void runValidityCheck() {
    validityCheck.create(this).process(externsRoot, jsRoot);
  }

  /** Runs custom passes that are designated to run at a particular time. */
  private void runCustomPasses(CustomPassExecutionTime executionTime) {
    if (options.customPasses != null) {
      Tracer t = newTracer("runCustomPasses");
      try {
        for (CompilerPass p : options.customPasses.get(executionTime)) {
          process(p);
        }
      } finally {
        stopTracer(t, "runCustomPasses");
      }
    }
  }

  @Override
  final void beforePass(String passName) {
    super.beforePass(passName);
  }

  @Override
  final void afterPass(String passName) {
    super.afterPass(passName);
    this.maybePrintSourceAfterEachPass(passName);
  }

  private void maybePrintSourceAfterEachPass(String passName) {
    if (!options.printSourceAfterEachPass) {
      return;
    }

    String currentJsSource = getCurrentJsSource();
    if (currentJsSource.equals(this.lastJsSource)) {
      return;
    }

    if (this.isDebugLoggingEnabled()) {
      try (LogFile log =
          this.createOrReopenIndexedLog(this.getClass(), "source_after_pass", passName)) {
        log.log(currentJsSource);
      }
    } else {
      System.err.println();
      System.err.println("// " + passName + " yields:");
      System.err.println("// ************************************");
      System.err.println(currentJsSource);
      System.err.println("// ************************************");
    }

    this.lastJsSource = currentJsSource;
  }

  final String getCurrentJsSource() {
    this.resetAndIntitializeSourceMap();

    List<String> fileNameRegexList = options.filesToPrintAfterEachPassRegexList;
    List<String> moduleNameRegexList = options.chunksToPrintAfterEachPassRegexList;
    final Set<String> qnameSet = new LinkedHashSet<>(options.qnameUsesToPrintAfterEachPassList);
    StringBuilder builder = new StringBuilder();

    if (fileNameRegexList.isEmpty() && moduleNameRegexList.isEmpty() && qnameSet.isEmpty()) {
      return toSource();
    }
    if (!fileNameRegexList.isEmpty()) {
      checkNotNull(externsRoot);
      checkNotNull(jsRoot);
      for (Node r : ImmutableList.of(externsRoot, jsRoot)) {
        for (Node fileNode = r.getFirstChild(); fileNode != null; fileNode = fileNode.getNext()) {
          String fileName = fileNode.getSourceFileName();
          for (String regex : fileNameRegexList) {
            if (fileName.matches(regex)) {
              String source = "// " + fileName + "\n" + toSource(fileNode);
              builder.append(source);
              break;
            }
          }
        }
      }
      if (builder.length() == 0) {
        builder.append("// No files matched any of: ").append(fileNameRegexList);
      }
    }
    if (!moduleNameRegexList.isEmpty()) {
      for (JSChunk jsModule : getModules()) {
        for (String regex : moduleNameRegexList) {
          if (jsModule.getName().matches(regex)) {
            String source = "// module '" + jsModule.getName() + "'\n" + toSource(jsModule);
            builder.append(source);
            break;
          }
        }
      }
      if (builder.length() == 0) {
        throw new RuntimeException("No modules matched any of: " + moduleNameRegexList);
      }
    }
    if (!qnameSet.isEmpty()) {
      // Keep track of how a qualified name we're interested in gets renamed.
      // Note that a qname can actually be renamed multiple times during compilation,
      // so we need a multimap here.
      final Multimap<String, String> originalToNewQNameMap = LinkedHashMultimap.create();
      // Get a stream of top level statement nodes that contain at least one reference to one of the
      // qnames we are interested in. Take note of relevant renamings while we're at it.
      final Stream<Node> statementStream =
          getTopLevelStatements(jsRoot)
              .filter(
                  (Node statement) ->
                      // filter to just those statements containing interesting qnames
                      NodeUtil.has(
                          statement,
                          (Node n) -> {
                            checkNotNull(n);
                            if (!n.isQualifiedName()) {
                              return false;
                            }
                            String qname = n.getQualifiedName();
                            if (qnameSet.contains(qname)) {
                              return true;
                            }
                            String originalQname = n.getOriginalQualifiedName();
                            if (originalQname != null && qnameSet.contains(originalQname)) {
                              originalToNewQNameMap.put(originalQname, qname);
                              return true;
                            } else {
                              return false;
                            }
                          },
                          node -> true));
      builder.append("//\n");
      builder.append("// closure-compiler: Printing all of the top-level statements\n");
      builder.append("// that contain references to these qualified names.\n");
      builder.append("//\n");
      for (String qname : qnameSet) {
        builder.append(SimpleFormat.format("// '%s'\n", qname));
        for (String newName : originalToNewQNameMap.get(qname)) {
          builder.append(SimpleFormat.format("// '%s' (originally '%s')\n", newName, qname));
        }
      }
      builder.append("//\n");
      statementStream.forEach(
          (Node statement) ->
              builder
                  .append(SimpleFormat.format("// %s\n", statement.getLocation()))
                  .append(toSource(statement))
                  .append("\n"));
    }
    return builder.toString();
  }

  private Stream<Node> getTopLevelStatements(Node root) {
    Stream.Builder<Node> builder = Stream.builder();
    NodeTraversal.traverse(
        this,
        root,
        new AbstractPreOrderCallback() {
          @Override
          public boolean shouldTraverse(NodeTraversal t, Node n, Node parent) {
            if (NodeUtil.isStatement(n)) {
              builder.add(n);
              return false;
            } else {
              return true;
            }
          }
        });
    return builder.build();
  }

  @Override
  @Nullable
  public final Node getScriptNode(String filename) {
    return scriptNodeByFilename.get(checkNotNull(filename));
  }

  /** Returns a new tracer for the given pass name. */
  Tracer newTracer(String passName) {
    String comment = passName + (recentChange.hasCodeChanged() ? " on recently changed AST" : "");
    if (options.getTracerMode().isOn() && tracker != null) {
      tracker.recordPassStart(passName, true);
    }
    return new Tracer("Compiler", comment);
  }

  void stopTracer(Tracer t, String passName) {
    long result = t.stop();
    if (options.getTracerMode().isOn() && tracker != null) {
      tracker.recordPassStop(passName, result);
    }
  }

  /** Returns the result of the compilation. */
  public Result getResult() {
    Set<SourceFile> transpiledFiles = new HashSet<>();
    if (jsRoot != null) {
      for (Node scriptNode = jsRoot.getFirstChild();
          scriptNode != null;
          scriptNode = scriptNode.getNext()) {
        if (scriptNode.getBooleanProp(Node.TRANSPILED)) {
          transpiledFiles.add(getSourceFileByName(scriptNode.getSourceFileName()));
        }
      }
    }
    return new Result(
        getErrors(),
        getWarnings(),
        this.variableMap,
        this.propertyMap,
        null,
        this.stringMap,
        this.instrumentationMapping,
        this.sourceMap,
        this.externExports,
        this.cssNames,
        this.idGeneratorMap,
        transpiledFiles);
  }

  /** Returns the list of errors (never null). */
  public ImmutableList<JSError> getErrors() {
    return (errorManager == null) ? ImmutableList.of() : errorManager.getErrors();
  }

  /** Returns the list of warnings (never null). */
  public ImmutableList<JSError> getWarnings() {
    return (errorManager == null) ? ImmutableList.of() : errorManager.getWarnings();
  }

  @Override
  public Node getRoot() {
    return externAndJsRoot;
  }

  @Override
  FeatureSet getFeatureSet() {
    return featureSet;
  }

  @Override
  void setFeatureSet(FeatureSet fs) {
    featureSet = fs;
  }

  @Override
  UniqueIdSupplier getUniqueIdSupplier() {
    return this.uniqueIdSupplier;
  }

  /** Creates a new id for making unique names. */
  @Deprecated
  private int nextUniqueNameId() {
    return uniqueNameId++;
  }

  /** Resets the unique name id counter */
  @Deprecated
  @VisibleForTesting
  void resetUniqueNameId() {
    uniqueNameId = 0;
  }

  /**
   * Legacy supplier for getting unique names.
   *
   * @deprecated This is deprecated because this supplier fails to generate unique Ids across input
   *     files when used for tagged template literal transpilation. Kindly use the new {@code
   *     UniqueIdSupplier} instead.
   */
  @Deprecated
  @Override
  com.google.common.base.Supplier<String> getUniqueNameIdSupplier() {
    return () -> String.valueOf(Compiler.this.nextUniqueNameId());
  }

  @Override
  boolean areNodesEqualForInlining(Node n1, Node n2) {
    if (options.shouldAmbiguateProperties() || options.shouldDisambiguateProperties()) {
      // The type based optimizations require that type information is preserved
      // during other optimizations.
      return n1.isEquivalentToTyped(n2);
    } else {
      return n1.isEquivalentTo(n2);
    }
  }

  // ------------------------------------------------------------------------
  // Inputs
  // ------------------------------------------------------------------------

  // TODO(nicksantos): Decide which parts of these belong in an AbstractCompiler
  // interface, and which ones should always be injected.

  @Override
  public CompilerInput getInput(InputId id) {
    // TODO(bradfordcsmith): Allowing null id is less ideal. Add checkNotNull(id) here and fix
    // call sites that break.
    if (id == null) {
      return null;
    }
    return inputsById.get(id);
  }

  private CompilerInput putCompilerInput(CompilerInput input) {
    input.setCompiler(this);
    return inputsById.put(input.getInputId(), input);
  }

  /**
   * Gets the graph of JS source modules.
   *
   * <p>Returns null if {@code #init} or {@code #initModules} hasn't been called yet. Otherwise, the
   * result is always a module graph, even in the degenerate case where there's only one module.
   */
  @Nullable
  @Override
  JSChunkGraph getModuleGraph() {
    return moduleGraph;
  }

  /**
   * Gets the JS source modules in dependency order.
   *
   * <p>Returns null if {@code #init} or {@code #initModules} hasn't been called yet. Otherwise, the
   * result is always non-empty, even in the degenerate case where there's only one module.
   */
  @Nullable
  public Iterable<JSChunk> getModules() {
    return moduleGraph != null ? moduleGraph.getAllChunks() : null;
  }

  @Override
  public void clearJSTypeRegistry() {
    typeRegistry = null;
    typeValidator = null;
    abstractInterpreter = null;
  }

  @Override
  public boolean isTypeRegistryCleared() {
    return typeCheckingHasRun && typeRegistry == null;
  }

  @Override
  public JSTypeRegistry getTypeRegistry() {
    if (typeRegistry == null) {
      checkState(
          !this.hasTypeCheckingRun(),
          // This could throw when calling "getTypeRegistry()" during the optimizations phase when
          // JSTypes have been converted to optimization colors
          "Attempted to re-initialize JSTypeRegistry after it had been cleared");
      typeRegistry = new JSTypeRegistry(oldErrorReporter, forwardDeclaredTypes);
    }
    return typeRegistry;
  }

  @Override
  public ColorRegistry getColorRegistry() {
    return checkNotNull(colorRegistry, "Color registry has not been initialized yet");
  }

  @Override
  public void setColorRegistry(ColorRegistry colorRegistry) {
    checkState(this.runtimeLibraryTypedAsts != null);
    this.colorRegistry = colorRegistry;
  }

  @Override
  public void forwardDeclareType(String typeName) {
    forwardDeclaredTypes.add(typeName);
  }

  @Override
  void setTypeCheckingHasRun(boolean hasRun) {
    this.typeCheckingHasRun = hasRun;
  }

  @Override
  public boolean hasTypeCheckingRun() {
    return this.typeCheckingHasRun;
  }

  @Override
  public boolean hasOptimizationColors() {
    return this.colorRegistry != null;
  }

  private TypedScopeCreator typedScopeCreator;

  @Override
  public ScopeCreator getTypedScopeCreator() {
    if (this.typedScopeCreator == null) {
      checkState(
          !this.hasTypeCheckingRun(),
          // This could throw when calling "getTypedScopeCreator()" during the optimizations phase
          // when JSTypes have been converted to optimization colors
          "Attempted to re-initialize TypedScopeCreator after it had been cleared");
      this.typedScopeCreator = new TypedScopeCreator(this);
    }
    return this.typedScopeCreator;
  }

  @Override
  void clearTypedScopeCreator() {
    this.typedScopeCreator = null;
  }

  DefaultPassConfig ensureDefaultPassConfig() {
    PassConfig passes = getPassConfig().getBasePassConfig();
    checkState(
        passes instanceof DefaultPassConfig,
        "PassConfigs must eventually delegate to the DefaultPassConfig");
    return (DefaultPassConfig) passes;
  }

  public SymbolTable buildKnownSymbolTable() {
    SymbolTable symbolTable = new SymbolTable(this, getTypeRegistry());

    if (this.typedScopeCreator != null) {
      symbolTable.addScopes(this.typedScopeCreator.getAllMemoizedScopes());
      symbolTable.addSymbolsFrom(this.typedScopeCreator);
    } else {
      symbolTable.findScopes(externsRoot, jsRoot);
    }

    GlobalNamespace globalNamespace = new GlobalNamespace(this, this.externsRoot, this.jsRoot);
    symbolTable.addSymbolsFrom(globalNamespace);

    ReferenceCollector refCollector =
        new ReferenceCollector(
            this, ReferenceCollector.DO_NOTHING_BEHAVIOR, new SyntacticScopeCreator(this));
    refCollector.process(getRoot());
    symbolTable.addSymbolsFrom(refCollector);

    PreprocessorSymbolTable preprocessorSymbolTable =
        ensureDefaultPassConfig().getPreprocessorSymbolTable();
    if (preprocessorSymbolTable != null) {
      symbolTable.addSymbolsFrom(preprocessorSymbolTable);
    }

    symbolTable.flattenGoogModuleExports();
    symbolTable.fillNamespaceReferences();
    symbolTable.fillPropertyScopes();
    symbolTable.fillThisReferences(externsRoot, jsRoot);
    symbolTable.fillPropertySymbols(externsRoot, jsRoot);
    symbolTable.fillSuperReferences(externsRoot, jsRoot);
    symbolTable.fillJSDocInfo(externsRoot, jsRoot);
    symbolTable.fillSymbolVisibility(externsRoot, jsRoot);
    symbolTable.fillGoogProvideModuleRequires(externsRoot, jsRoot);
    symbolTable.removeGeneratedSymbols();

    return symbolTable;
  }

  private TypedScope topScope = null;

  @Override
  public TypedScope getTopScope() {
    return this.topScope;
  }

  @Override
  void setTopScope(TypedScope x) {
    checkState(x == null || x.getParent() == null, x);
    this.topScope = x;
  }

  @Override
  public ReverseAbstractInterpreter getReverseAbstractInterpreter() {
    if (abstractInterpreter == null) {
      ChainableReverseAbstractInterpreter interpreter =
          new SemanticReverseAbstractInterpreter(getTypeRegistry());
      if (options.closurePass) {
        interpreter =
            new ClosureReverseAbstractInterpreter(getTypeRegistry()).append(interpreter).getFirst();
      }
      abstractInterpreter = interpreter;
    }
    return abstractInterpreter;
  }

  @Override
  TypeValidator getTypeValidator() {
    if (typeValidator == null) {
      checkState(
          !this.hasTypeCheckingRun(),
          // This could throw when calling "getTypeValidator()" during the optimizations phase when
          // JSTypes have been converted to optimization colors
          "Attempted to re-initialize TypeValidator after it had been cleared");
      typeValidator = new TypeValidator(this);
    }
    return typeValidator;
  }

  @Override
  public Iterable<TypeMismatch> getTypeMismatches() {
    if (this.typeCheckingHasRun) {
      return getTypeValidator().getMismatches();
    }
    throw new RuntimeException("Can't ask for type mismatches before type checking.");
  }

  @Override
  StaticScope getTranspilationNamespace() {
    if (this.transpilationNamespace == null) {
      // Note: this also needs to happen /after/ the runtime libraries are injected, or we will not
      // be able to find the runtime library code.
      checkState(
          !getLifeCycleStage().isNormalized(),
          "cannot init transpilation namespace after optimizations phase, or information may be"
              + " lost");

      GlobalNamespace gn = new GlobalNamespace(this, this.getExternsRoot(), this.getJsRoot());
      // Exclude user-defined code names, to save memory and help prevent misuse.
      gn.setShouldTraverseScriptPredicate(
          (Node script) -> script.isFromExterns() || script.isFirstChildOf(this.getJsRoot()));
      gn.getNameForest(); // ensure namespace is generated
      // wrap GlobalNamespace to prevent looking up extra information in GlobalNamespace
      this.transpilationNamespace = gn;
    }

    return this.transpilationNamespace;
  }

  public void maybeSetTracker() {
    if (!options.getTracerMode().isOn()) {
      return;
    }

    tracker = new PerformanceTracker(externsRoot, jsRoot, options.getTracerMode());
    addChangeHandler(tracker.getCodeChangeHandler());
  }

  void initializeModuleLoader() {
    ModuleResolverFactory moduleResolverFactory = null;

    switch (options.getModuleResolutionMode()) {
      case BROWSER:
        moduleResolverFactory = BrowserModuleResolver.FACTORY;
        break;
      case NODE:
        // processJsonInputs requires a module loader to already be defined
        // so we redefine it afterwards with the package.json inputs
        moduleResolverFactory =
            new NodeModuleResolver.Factory(processJsonInputs(moduleGraph.getAllInputs()));
        break;
      case WEBPACK:
        moduleResolverFactory = new WebpackModuleResolver.Factory(inputPathByWebpackId);
        break;
      case BROWSER_WITH_TRANSFORMED_PREFIXES:
        moduleResolverFactory =
            new BrowserWithTransformedPrefixesModuleResolver.Factory(
                options.getBrowserResolverPrefixReplacements());
        break;
    }

    this.moduleLoader =
        ModuleLoader.builder()
            .setModuleRoots(options.moduleRoots)
            .setInputs(moduleGraph.getAllInputs())
            .setFactory(moduleResolverFactory)
            .setPathResolver(PathResolver.RELATIVE)
            .setPathEscaper(options.getPathEscaper())
            .build();
  }

  // ------------------------------------------------------------------------
  // Parsing
  // ------------------------------------------------------------------------

  /**
   * Parses the externs and main inputs.
   *
   * @return A synthetic root node whose two children are the externs root and the main root
   */
  Node parseInputs() {
    boolean devMode = options.devMode != DevMode.OFF;

    // If old roots exist (we are parsing a second time), detach each of the
    // individual file parse trees.
    externsRoot.detachChildren();
    jsRoot.detachChildren();
    scriptNodeByFilename.clear();

    Tracer tracer = newTracer(PassNames.PARSE_INPUTS);
    beforePass(PassNames.PARSE_INPUTS);

    try {
      // Parse externs sources.
      if (options.numParallelThreads > 1) {
        new PrebuildAst(this, options.numParallelThreads).prebuild(externs);
      }
      for (CompilerInput input : externs) {
        Node n = checkNotNull(input.getAstRoot(this));
        if (hasErrors()) {
          return null;
        }
        externsRoot.addChildToBack(n);
        scriptNodeByFilename.put(input.getSourceFile().getName(), n);
      }

      if (options.transformAMDToCJSModules) {
        processAMDModules(moduleGraph.getAllInputs());
      }

      if (options.getLanguageIn().toFeatureSet().has(Feature.MODULES)
          || options.getProcessCommonJSModules()) {
        initializeModuleLoader();
      } else {
        // Use an empty module loader if we're not actually dealing with modules.
        this.moduleLoader = ModuleLoader.EMPTY;
      }

      if (options.getDependencyOptions().needsManagement()) {
        findModulesFromEntryPoints(
            options.getLanguageIn().toFeatureSet().has(Feature.MODULES),
            options.getProcessCommonJSModules());
      } else if (options.needsTranspilationFrom(FeatureSet.ES2015_MODULES)
          || options.getProcessCommonJSModules()) {
        if (options.getLanguageIn().toFeatureSet().has(Feature.MODULES)) {
          parsePotentialModules(moduleGraph.getAllInputs());
        }

        // Build a map of module identifiers for any input which provides no namespace.
        // These files could be imported modules which have no exports, but do have side effects.
        Map<String, CompilerInput> inputModuleIdentifiers = new HashMap<>();
        for (CompilerInput input : moduleGraph.getAllInputs()) {
          if (input.getKnownProvides().isEmpty()) {
            ModulePath modPath = moduleLoader.resolve(input.getSourceFile().getName());
            inputModuleIdentifiers.put(modPath.toModuleName(), input);
          }
        }

        // Find out if any input attempted to import a module that had no exports.
        // In this case we must force module rewriting to occur on the imported file
        Map<String, CompilerInput> inputsToRewrite = new HashMap<>();
        for (CompilerInput input : moduleGraph.getAllInputs()) {
          for (String require : input.getKnownRequiredSymbols()) {
            if (inputModuleIdentifiers.containsKey(require)
                && !inputsToRewrite.containsKey(require)) {
              inputsToRewrite.put(require, inputModuleIdentifiers.get(require));
            }
          }
        }

        for (CompilerInput input : inputsToRewrite.values()) {
          input.setJsModuleType(ModuleType.IMPORTED_SCRIPT);
        }
      }

      if (this.moduleLoader != null) {
        this.moduleLoader.setErrorHandler(this);
      }

      orderInputs();

      // If in IDE mode, we ignore the error and keep going.
      if (hasErrors()) {
        return null;
      }

      // Build the AST.
      if (options.numParallelThreads > 1) {
        new PrebuildAst(this, options.numParallelThreads).prebuild(moduleGraph.getAllInputs());
      }

      for (CompilerInput input : moduleGraph.getAllInputs()) {
        Node n = checkNotNull(input.getAstRoot(this));
        if (devMode) {
          runValidityCheck();
          if (hasErrors()) {
            return null;
          }
        }

        // TODO(johnlenz): we shouldn't need to check both isExternExportsEnabled and
        // externExportsPath.
        if (options.sourceMapOutputPath != null
            || options.isExternExportsEnabled()
            || options.externExportsPath != null
            || !options.replaceStringsFunctionDescriptions.isEmpty()) {

          // Annotate the nodes in the tree with information from the
          // input file. This information is used to construct the SourceMap.
          SourceInformationAnnotator sia =
              DevMode.OFF.equals(options.devMode)
                  ? SourceInformationAnnotator.create()
                  : SourceInformationAnnotator.createWithAnnotationChecks(input.getName());
          NodeTraversal.traverse(this, n, sia);
        }

        if (NodeUtil.isFromTypeSummary(n)) {
          input.setIsExtern();
          externsRoot.addChildToBack(n);
        } else {
          jsRoot.addChildToBack(n);
        }
        scriptNodeByFilename.put(input.getSourceFile().getName(), n);
      }

      if (hasErrors()) {
        return null;
      }
      return externAndJsRoot;
    } finally {
      afterPass(PassNames.PARSE_INPUTS);
      stopTracer(tracer, PassNames.PARSE_INPUTS);

      // Save on memory. Any future calls to "createInputConsideringTypedAstFilesystem" will throw.
      // TODO(lharker): do we actually need the synthetic externs file in stage 2?
      if (this.typedAstFilesystem != null) {
        this.typedAstFilesystem =
            ImmutableMap.of(
                SYNTHETIC_EXTERNS_FILE, this.typedAstFilesystem.get(SYNTHETIC_EXTERNS_FILE));
      }
    }
  }

  void orderInputsWithLargeStack() {
    runInCompilerThread(
        () -> {
          Tracer tracer = newTracer("orderInputsWithLargeStack");
          try {
            orderInputs();
          } finally {
            stopTracer(tracer, "orderInputsWithLargeStack");
          }
          return null;
        });
  }

  void orderInputs() {
    maybeDoThreadedParsing();

    // Before dependency pruning, save a copy of the original inputs to use for externs hoisting.
    ImmutableList<CompilerInput> originalInputs = ImmutableList.copyOf(moduleGraph.getAllInputs());

    // Externs must be marked before dependency management since it needs to know what is an extern.
    markExterns(originalInputs);

    // Check if the sources need to be re-ordered.
    boolean staleInputs = false;
    if (options.getDependencyOptions().needsManagement()) {
      for (CompilerInput input : moduleGraph.getAllInputs()) {
        // Forward-declare all the provided types, so that they
        // are not flagged even if they are dropped from the process.
        for (String provide : input.getProvides()) {
          forwardDeclareType(provide);
        }
      }

      try {
        moduleGraph.manageDependencies(this, options.getDependencyOptions());
        staleInputs = true;
      } catch (MissingProvideException e) {
        report(JSError.make(MISSING_ENTRY_ERROR, e.getMessage()));
      } catch (MissingChunkException e) {
        report(JSError.make(MISSING_MODULE_ERROR, e.getMessage()));
      }
    }
    hoistExterns(originalInputs);

    // Manage dependencies may move weak sources around, and end up with empty modules.
    fillEmptyModules(getModules());
    hoistNoCompileFiles();

    if (staleInputs) {
      repartitionInputs();
    }
  }

  /**
   * Find modules by recursively traversing dependencies starting with the entry points.
   *
   * <p>Causes a regex parse of every file, and a full parse of every file reachable from the entry
   * points (which would be required by later compilation passes regardless).
   *
   * <p>If the dependency mode is set to PRUNE_LEGACY, inputs which the regex parse does not
   * identify as ES modules and which do not contain any provide statements are considered to be
   * additional entry points.
   */
  private void findModulesFromEntryPoints(
      boolean supportEs6Modules, boolean supportCommonJSModules) {
    maybeDoThreadedParsing();
    List<CompilerInput> entryPoints = new ArrayList<>();
    Map<String, CompilerInput> inputsByProvide = new HashMap<>();
    Map<String, CompilerInput> inputsByIdentifier = new HashMap<>();
    for (CompilerInput input : moduleGraph.getAllInputs()) {
      Iterable<String> provides =
          Iterables.filter(input.getProvides(), p -> !p.startsWith("module$"));
      if (!options.getDependencyOptions().shouldDropMoochers() && Iterables.isEmpty(provides)) {
        entryPoints.add(input);
      }
      inputsByIdentifier.put(
          ModuleIdentifier.forFile(input.getPath().toString()).toString(), input);
      for (String provide : provides) {
        inputsByProvide.put(provide, input);
      }
    }
    for (ModuleIdentifier moduleIdentifier : options.getDependencyOptions().getEntryPoints()) {
      CompilerInput input = inputsByProvide.get(moduleIdentifier.toString());
      if (input == null) {
        input = inputsByIdentifier.get(moduleIdentifier.toString());
      }
      if (input != null) {
        entryPoints.add(input);
      }
    }

    Set<CompilerInput> workingInputSet = Sets.newHashSet(moduleGraph.getAllInputs());
    for (CompilerInput entryPoint : entryPoints) {
      findModulesFromInput(
          entryPoint,
          /* wasImportedByModule = */ false,
          workingInputSet,
          inputsByIdentifier,
          inputsByProvide,
          supportEs6Modules,
          supportCommonJSModules);
    }
  }

  /** Traverse an input's dependencies to find additional modules. */
  private void findModulesFromInput(
      CompilerInput input,
      boolean wasImportedByModule,
      Set<CompilerInput> inputs,
      Map<String, CompilerInput> inputsByIdentifier,
      Map<String, CompilerInput> inputsByProvide,
      boolean supportEs6Modules,
      boolean supportCommonJSModules) {
    if (!inputs.remove(input)) {
      // It's possible for a module to be included as both a script
      // and a module in the same compilation. In these cases, it should
      // be forced to be a module.
      if (wasImportedByModule && input.getJsModuleType() == ModuleType.NONE) {
        input.setJsModuleType(ModuleType.IMPORTED_SCRIPT);
      }
      return;
    }

    FindModuleDependencies findDeps =
        new FindModuleDependencies(
            this, supportEs6Modules, supportCommonJSModules, inputPathByWebpackId);
    findDeps.process(checkNotNull(input.getAstRoot(this)));

    // If this input was imported by another module, it is itself a module
    // so we force it to be detected as such.
    if (wasImportedByModule && input.getJsModuleType() == ModuleType.NONE) {
      input.setJsModuleType(ModuleType.IMPORTED_SCRIPT);
    }
    this.moduleTypesByName.put(input.getPath().toModuleName(), input.getJsModuleType());

    Iterable<String> allDeps =
        Iterables.concat(
            input.getRequiredSymbols(), input.getDynamicRequires(), input.getTypeRequires());
    for (String requiredNamespace : allDeps) {
      CompilerInput requiredInput = null;
      boolean requiredByModuleImport = false;
      if (inputsByProvide.containsKey(requiredNamespace)) {
        requiredInput = inputsByProvide.get(requiredNamespace);
      } else if (inputsByIdentifier.containsKey(requiredNamespace)) {
        requiredByModuleImport = true;
        requiredInput = inputsByIdentifier.get(requiredNamespace);
      }

      if (requiredInput != null) {
        findModulesFromInput(
            requiredInput,
            requiredByModuleImport,
            inputs,
            inputsByIdentifier,
            inputsByProvide,
            supportEs6Modules,
            supportCommonJSModules);
      }
    }
  }

  /** Hoists inputs with the @externs annotation into the externs list. */
  void hoistExterns(ImmutableList<CompilerInput> originalInputs) {
    boolean staleInputs = false;

    for (CompilerInput input : originalInputs) {
      if (hoistIfExtern(input)) {
        staleInputs = true;
      }
    }
    if (staleInputs) {
      repartitionInputs();
    }
  }

  /**
   * Hoists a compiler input to externs if it contains the @externs annotation. Return whether or
   * not the given input was hoisted.
   */
  private boolean hoistIfExtern(CompilerInput input) {
    if (input.getHasExternsAnnotation()) {
      // If the input file is explicitly marked as an externs file, then move it out of the main
      // JS root and put it with the other externs.
      Node root = input.getAstRoot(this);
      externsRoot.addChildToBack(root);
      scriptNodeByFilename.put(input.getSourceFile().getName(), root);

      JSChunk module = input.getChunk();
      if (module != null) {
        module.remove(input);
      }

      externs.add(input);
      return true;
    }
    return false;
  }

  /**
   * Marks inputs with the @externs annotation as an Extern source file type. This is so that
   * externs marking can be done before dependency management, and externs hoisting done after
   * dependency management.
   */
  private void markExterns(ImmutableList<CompilerInput> originalInputs) {
    for (CompilerInput input : originalInputs) {
      if (input.getHasExternsAnnotation()) {
        input.setIsExtern();
      }
    }
  }

  /** Hoists inputs with the @nocompile annotation out of the inputs. */
  void hoistNoCompileFiles() {
    boolean staleInputs = false;
    maybeDoThreadedParsing();
    // Iterate a copy because hoisting modifies what we're iterating over.
    for (CompilerInput input : ImmutableList.copyOf(moduleGraph.getAllInputs())) {
      if (input.getHasNoCompileAnnotation()) {
        input.getChunk().remove(input);
        staleInputs = true;
      }
    }

    if (staleInputs) {
      repartitionInputs();
    }
  }

  private void maybeDoThreadedParsing() {
    if (options.numParallelThreads > 1) {
      new PrebuildDependencyInfo(options.numParallelThreads).prebuild(moduleGraph.getAllInputs());
    }
  }

  private void repartitionInputs() {
    fillEmptyModules(getModules());
    rebuildInputsFromModules();
  }

  /**
   * Transforms JSON files to a module export that closure compiler can process and keeps track of
   * any "main" entries in package.json files.
   */
  Map<String, String> processJsonInputs(Iterable<CompilerInput> inputsToProcess) {
    RewriteJsonToModule rewriteJson = new RewriteJsonToModule(this);
    for (CompilerInput input : inputsToProcess) {
      if (!input.getSourceFile().getName().endsWith(".json")) {
        continue;
      }

      input.setCompiler(this);
      try {
        // JSON objects need wrapped in parens to parse properly
        input.getSourceFile().setCodeDeprecated("(" + input.getSourceFile().getCode() + ")");
      } catch (IOException e) {
        continue;
      }

      Node root = checkNotNull(input.getAstRoot(this));
      input.setJsModuleType(ModuleType.JSON);
      rewriteJson.process(null, root);
    }
    return rewriteJson.getPackageJsonMainEntries();
  }

  private List<CompilerInput> parsePotentialModules(Iterable<CompilerInput> inputsToProcess) {
    List<CompilerInput> filteredInputs = new ArrayList<>();
    for (CompilerInput input : inputsToProcess) {
      // Only process files that are detected as ES6 modules
      if (!options.getDependencyOptions().shouldPrune()
          || !JsFileRegexParser.isSupported()
          || "es6".equals(input.getLoadFlags().get("module"))) {
        filteredInputs.add(input);
      }
    }
    if (options.numParallelThreads > 1) {
      new PrebuildAst(this, options.numParallelThreads).prebuild(filteredInputs);
    }
    for (CompilerInput input : filteredInputs) {
      input.setCompiler(this);
      // Call getRequires to force regex-based dependency parsing to happen.
      input.getRequires();
      input.setJsModuleType(ModuleType.ES6);
    }
    return filteredInputs;
  }

  /** Transforms AMD to CJS modules */
  void processAMDModules(Iterable<CompilerInput> inputs) {
    for (CompilerInput input : inputs) {
      input.setCompiler(this);
      Node root = checkNotNull(input.getAstRoot(this));
      new TransformAMDToCJSModule(this).process(null, root);
    }
  }

  /** Allow subclasses to override the default CompileOptions object. */
  protected CompilerOptions newCompilerOptions() {
    return new CompilerOptions();
  }

  void initCompilerOptionsIfTesting() {
    if (options == null) {
      // initialization for tests that don't initialize the compiler
      // by the normal mechanisms.
      initOptions(newCompilerOptions());
    }
  }

  @Override
  public Node parseSyntheticCode(String fileName, String js) {
    initCompilerOptionsIfTesting();
    SourceFile source = SourceFile.fromCode(" [synthetic:" + fileName + "] ", js);
    addFilesToSourceMap(ImmutableList.of(source));
    return parseCodeHelper(source);
  }

  @Override
  @VisibleForTesting
  Node parseTestCode(String js) {
    initCompilerOptionsIfTesting();
    initBasedOnOptions();
    return parseCodeHelper(SourceFile.fromCode("[testcode]", js));
  }

  @Override
  @VisibleForTesting
  Node parseTestCode(ImmutableList<String> code) {
    initCompilerOptionsIfTesting();
    initBasedOnOptions();
    return parseCodeHelper(
        Streams.mapWithIndex(
                code.stream(), (value, index) -> SourceFile.fromCode("testcode" + index, value))
            .collect(Collectors.toList()));
  }

  private Node parseCodeHelper(SourceFile src) {
    CompilerInput input = new CompilerInput(src);
    putCompilerInput(input);
    Node root = input.getAstRoot(this);
    scriptNodeByFilename.put(input.getSourceFile().getName(), root);
    return checkNotNull(root);
  }

  private Node parseCodeHelper(List<SourceFile> srcs) {
    Node root = IR.root();
    for (SourceFile src : srcs) {
      root.addChildToBack(parseCodeHelper(src));
    }
    return root;
  }

  @Override
  ErrorReporter getDefaultErrorReporter() {
    return oldErrorReporter;
  }

  // ------------------------------------------------------------------------
  // Convert back to source code
  // ------------------------------------------------------------------------

  /** Converts the main parse tree back to JS code. */
  @Override
  public String toSource() {
    return runInCompilerThread(
        () -> {
          Tracer tracer = newTracer("toSource");
          try {
            CodeBuilder cb = new CodeBuilder();
            if (jsRoot != null) {
              int i = 0;
              if (options.shouldPrintExterns()) {
                for (Node scriptNode = externsRoot.getFirstChild();
                    scriptNode != null;
                    scriptNode = scriptNode.getNext()) {
                  toSource(cb, i++, scriptNode);
                }
              }
              for (Node scriptNode = jsRoot.getFirstChild();
                  scriptNode != null;
                  scriptNode = scriptNode.getNext()) {
                toSource(cb, i++, scriptNode);
              }
            }
            return cb.toString();
          } finally {
            stopTracer(tracer, "toSource");
          }
        });
  }

  /** Converts the parse tree for a module back to JS code. */
  public String toSource(final JSChunk module) {
    return runInCompilerThread(
        () -> {
          List<CompilerInput> inputs = module.getInputs();
          int numInputs = inputs.size();
          if (numInputs == 0) {
            return "";
          }
          CodeBuilder cb = new CodeBuilder();
          for (int i = 0; i < numInputs; i++) {
            Node scriptNode = inputs.get(i).getAstRoot(Compiler.this);
            if (scriptNode == null) {
              throw new IllegalArgumentException("Bad module: " + module.getName());
            }
            toSource(cb, i, scriptNode);
          }
          return cb.toString();
        });
  }

  /**
   * Writes out JS code from a root node. If printing input delimiters, this method will attach a
   * comment to the start of the text indicating which input the output derived from. If there were
   * any preserve annotations within the root's source, they will also be printed in a block comment
   * at the beginning of the output.
   */
  public void toSource(final CodeBuilder cb, final int inputSeqNum, final Node root) {
    runInCompilerThread(
        () -> {
          if (options.printInputDelimiter) {
            if ((cb.getLength() > 0) && !cb.endsWith("\n")) {
              cb.append("\n"); // Make sure that the label starts on a new line
            }
            checkState(root.isScript());

            String delimiter = options.inputDelimiter;

            String inputName = root.getInputId().getIdName();
            String sourceName = root.getSourceFileName();
            checkState(sourceName != null);
            checkState(!sourceName.isEmpty());

            delimiter =
                delimiter
                    .replace("%name%", Matcher.quoteReplacement(inputName))
                    .replace("%num%", String.valueOf(inputSeqNum))
                    .replace("%n%", "\n");

            cb.append(delimiter).append("\n");
          }
          if (root.getJSDocInfo() != null) {
            String license = root.getJSDocInfo().getLicense();
            if (license != null && cb.addLicense(license)) {
              cb.append("/*\n").append(license).append("*/\n");
            }
          }

          // If there is a valid source map, then indicate to it that the current
          // root node's mappings are offset by the given string builder buffer.
          if (options.sourceMapOutputPath != null) {
            sourceMap.setStartingPosition(cb.getLineIndex(), cb.getColumnIndex());
          }

          // if LanguageMode is strict, only print 'use strict'
          // for the first input file
          String code = toSource(root, sourceMap, inputSeqNum == 0);
          if (!code.isEmpty()) {
            cb.append(code);

            // In order to avoid parse ambiguity when files are concatenated
            // together, all files should end in a semi-colon. Do a quick
            // heuristic check if there's an obvious semi-colon already there.
            int length = code.length();
            char lastChar = code.charAt(length - 1);
            char secondLastChar = length >= 2 ? code.charAt(length - 2) : '\0';
            boolean hasSemiColon = lastChar == ';' || (lastChar == '\n' && secondLastChar == ';');
            if (!hasSemiColon) {
              cb.append(";");
            }
          }
          return null;
        });
  }

  /** Generates JavaScript source code for an AST, doesn't generate source map info. */
  @Override
  public String toSource(Node n) {
    initCompilerOptionsIfTesting();
    return toSource(n, null, true);
  }

  /** Generates JavaScript source code for an AST. */
  private String toSource(Node n, SourceMap sourceMap, boolean firstOutput) {
    CodePrinter.Builder builder = new CodePrinter.Builder(n);
    builder.setCompilerOptions(options);
    builder.setSourceMap(sourceMap);
    builder.setTagAsTypeSummary(options.shouldGenerateTypedExterns());
    builder.setTagAsStrict(firstOutput && options.shouldEmitUseStrict());
    return builder.build();
  }

  /** Converts the parse tree for each input back to JS code. */
  public String[] toSourceArray() {
    return runInCompilerThread(
        () -> {
          Tracer tracer = newTracer("toSourceArray");
          try {
            int numInputs = moduleGraph.getInputCount();
            String[] sources = new String[numInputs];
            CodeBuilder cb = new CodeBuilder();
            int i = 0;
            for (CompilerInput input : moduleGraph.getAllInputs()) {
              Node scriptNode = input.getAstRoot(Compiler.this);
              cb.reset();
              toSource(cb, i, scriptNode);
              sources[i] = cb.toString();
              i++;
            }
            return sources;
          } finally {
            stopTracer(tracer, "toSourceArray");
          }
        });
  }

  /** Converts the parse tree for each input in a module back to JS code. */
  public String[] toSourceArray(final JSChunk module) {
    return runInCompilerThread(
        () -> {
          List<CompilerInput> inputs = module.getInputs();
          int numInputs = inputs.size();
          if (numInputs == 0) {
            return new String[0];
          }

          String[] sources = new String[numInputs];
          CodeBuilder cb = new CodeBuilder();
          for (int i = 0; i < numInputs; i++) {
            Node scriptNode = inputs.get(i).getAstRoot(Compiler.this);
            if (scriptNode == null) {
              throw new IllegalArgumentException("Bad module input: " + inputs.get(i).getName());
            }

            cb.reset();
            toSource(cb, i, scriptNode);
            sources[i] = cb.toString();
          }
          return sources;
        });
  }

  /**
   * Stores a buffer of text to which more can be appended. This is just like a StringBuilder except
   * that we also track the number of lines.
   */
  public static class CodeBuilder {
    private final StringBuilder sb = new StringBuilder();
    private int lineCount = 0;
    private int colCount = 0;
    private final Set<String> uniqueLicenses = new HashSet<>();

    /** Removes all text, but leaves the line count unchanged. */
    void reset() {
      sb.setLength(0);
    }

    /** Appends the given string to the text buffer. */
    CodeBuilder append(String str) {
      sb.append(str);

      // Adjust the line and column information for the new text.
      int index = -1;
      int lastIndex = index;
      while ((index = str.indexOf('\n', index + 1)) >= 0) {
        ++lineCount;
        lastIndex = index;
      }

      if (lastIndex == -1) {
        // No new lines, append the new characters added.
        colCount += str.length();
      } else {
        colCount = str.length() - (lastIndex + 1);
      }

      return this;
    }

    /** Returns all text in the text buffer. */
    @Override
    public String toString() {
      return sb.toString();
    }

    /** Returns the length of the text buffer. */
    public int getLength() {
      return sb.length();
    }

    /** Returns the (zero-based) index of the last line in the text buffer. */
    int getLineIndex() {
      return lineCount;
    }

    /** Returns the (zero-based) index of the last column in the text buffer. */
    int getColumnIndex() {
      return colCount;
    }

    /** Determines whether the text ends with the given suffix. */
    boolean endsWith(String suffix) {
      return (sb.length() > suffix.length())
          && suffix.equals(sb.substring(sb.length() - suffix.length()));
    }

    /** Adds a license and returns whether it is unique (has yet to be encountered). */
    boolean addLicense(String license) {
      return uniqueLicenses.add(license);
    }
  }

  // ------------------------------------------------------------------------
  // Optimizations
  // ------------------------------------------------------------------------

  void performTranspilationAndOptimizations() {
    checkState(options.shouldOptimize());
    // getOptimizations() also includes transpilation passes
    List<PassFactory> optimizations = getPassConfig().getOptimizations();
    if (optimizations.isEmpty()) {
      return;
    }

    phaseOptimizer = createPhaseOptimizer();
    phaseOptimizer.consume(optimizations);
    phaseOptimizer.process(externsRoot, jsRoot);
    phaseOptimizer = null;
  }

  void performFinalizations() {
    List<PassFactory> finalizations = getPassConfig().getFinalizations();
    if (finalizations.isEmpty()) {
      return;
    }

    phaseOptimizer = createPhaseOptimizer();
    phaseOptimizer.consume(finalizations);
    phaseOptimizer.process(externsRoot, jsRoot);
    phaseOptimizer = null;
  }

  @Override
  void setCssRenamingMap(CssRenamingMap map) {
    options.cssRenamingMap = map;
  }

  @Override
  CssRenamingMap getCssRenamingMap() {
    return options.cssRenamingMap;
  }

  /** Control Flow Analysis. */
  ControlFlowGraph<Node> computeCFG() {
    logger.fine("Computing Control Flow Graph");
    Tracer tracer = newTracer("computeCFG");
    ControlFlowAnalysis cfa = new ControlFlowAnalysis(this, true, false);
    process(cfa);
    stopTracer(tracer, "computeCFG");
    return cfa.getCfg();
  }

  private static final InputId SYNTHETIC_CODE_INPUT_ID = new InputId(" [synthetic:input] ");

  /**
   * Non-static because this file represents different content for every Compiler (and TSAN
   * complains if one instance is shared by all threads).
   */
  @VisibleForTesting
  final SourceFile SYNTHETIC_EXTERNS_FILE = SourceFile.fromCode(" [synthetic:externs] ", "");

  @Nullable private CompilerInput syntheticExternsInput; // matches SYNTHETIC_EXTERNS_FILE

  protected final RecentChange recentChange = new RecentChange();
  private final List<CodeChangeHandler> codeChangeHandlers = new ArrayList<>();
  private final Map<Class<?>, IndexProvider<?>> indexProvidersByType = new LinkedHashMap<>();

  @Override
  void addChangeHandler(CodeChangeHandler handler) {
    codeChangeHandlers.add(handler);
  }

  @Override
  void removeChangeHandler(CodeChangeHandler handler) {
    codeChangeHandlers.remove(handler);
  }

  @Override
  void addIndexProvider(IndexProvider<?> indexProvider) {
    Class<?> type = indexProvider.getType();
    if (indexProvidersByType.put(type, indexProvider) != null) {
      throw new IllegalStateException(
          "A provider is already registered for index of type " + type.getSimpleName());
    }
  }

  @SuppressWarnings("unchecked")
  @Override
  <T> T getIndex(Class<T> key) {
    IndexProvider<T> indexProvider = (IndexProvider<T>) indexProvidersByType.get(key);
    if (indexProvider == null) {
      return null;
    }
    return indexProvider.get();
  }

  protected Node getExternsRoot() {
    return externsRoot;
  }

  @Override
  protected Node getJsRoot() {
    return jsRoot;
  }

  /**
   * Some tests don't want to call the compiler "wholesale," they may not want to call check and/or
   * optimize. With this method, tests can execute custom optimization loops.
   */
  @VisibleForTesting
  void setPhaseOptimizer(PhaseOptimizer po) {
    this.phaseOptimizer = po;
  }

  @Override
  public int getChangeStamp() {
    return changeStamp;
  }

  @Override
  List<Node> getChangedScopeNodesForPass(String passName) {
    List<Node> changedScopeNodes = changeTimeline.getSince(passName);
    changeTimeline.mark(passName);
    return changedScopeNodes;
  }

  @Override
  List<Node> getDeletedScopeNodesForPass(String passName) {
    List<Node> deletedScopeNodes = deleteTimeline.getSince(passName);
    deleteTimeline.mark(passName);
    return deletedScopeNodes;
  }

  @Override
  public void incrementChangeStamp() {
    changeStamp++;
  }

  private Node getChangeScopeForNode(Node n) {
    /**
     * Compiler change reporting usually occurs after the AST change has already occurred. In the
     * case of node removals those nodes are already removed from the tree and so have no parent
     * chain to walk. In these situations changes are reported instead against what (used to be)
     * their parent. If that parent is itself a script node then it's important to be able to
     * recognize it as the enclosing scope without first stepping to its parent as well.
     */
    if (n.isScript()) {
      return n;
    }

    Node enclosingScopeNode = NodeUtil.getEnclosingChangeScopeRoot(n.getParent());
    if (enclosingScopeNode == null) {
      throw new IllegalStateException(
          "An enclosing scope is required for change reports but node " + n + " doesn't have one.");
    }
    return enclosingScopeNode;
  }

  private void recordChange(Node n) {
    if (n.isDeleted()) {
      // Some complicated passes (like SmartNameRemoval) might both change and delete a scope in
      // the same pass, and they might even perform the change after the deletion because of
      // internal queueing. Just ignore the spurious attempt to mark changed after already marking
      // deleted. There's no danger of deleted nodes persisting in the AST since this is enforced
      // separately in ChangeVerifier.
      return;
    }

    n.setChangeTime(changeStamp);
    // Every code change happens at a different time
    changeStamp++;
    changeTimeline.add(n);
  }

  @Override
  boolean hasScopeChanged(Node n) {
    if (phaseOptimizer == null) {
      return true;
    }
    return phaseOptimizer.hasScopeChanged(n);
  }

  @Override
  public void reportChangeToChangeScope(Node changeScopeRoot) {
    checkState(changeScopeRoot.isScript() || changeScopeRoot.isFunction());
    recordChange(changeScopeRoot);
    notifyChangeHandlers();
  }

  @Override
  public void reportFunctionDeleted(Node n) {
    checkState(n.isFunction());
    n.setDeleted(true);
    changeTimeline.remove(n);
    deleteTimeline.add(n);
  }

  @Override
  public void reportChangeToEnclosingScope(Node n) {
    recordChange(getChangeScopeForNode(n));
    notifyChangeHandlers();
  }

  private void notifyChangeHandlers() {
    for (CodeChangeHandler handler : codeChangeHandlers) {
      handler.reportChange();
    }
  }

  @Override
  public CodingConvention getCodingConvention() {
    CodingConvention convention = options.getCodingConvention();
    convention = convention != null ? convention : defaultCodingConvention;
    return convention;
  }

  private LanguageMode getParserConfigLanguageMode(CompilerOptions.LanguageMode languageMode) {
    switch (languageMode) {
      case ECMASCRIPT3:
        return LanguageMode.ECMASCRIPT3;
      case ECMASCRIPT5:
      case ECMASCRIPT5_STRICT:
        return LanguageMode.ECMASCRIPT5;
      case ECMASCRIPT_2015:
        return LanguageMode.ECMASCRIPT_2015;
      case ECMASCRIPT_2016:
        return LanguageMode.ECMASCRIPT_2016;
      case ECMASCRIPT_2017:
        return LanguageMode.ECMASCRIPT_2017;
      case ECMASCRIPT_2018:
        return LanguageMode.ECMASCRIPT_2018;
      case ECMASCRIPT_2019:
        return LanguageMode.ECMASCRIPT_2019;
      case ECMASCRIPT_2020:
        return LanguageMode.ECMASCRIPT_2020;
      case ECMASCRIPT_2021:
        return LanguageMode.ECMASCRIPT_2021;
      case UNSUPPORTED:
        return LanguageMode.UNSUPPORTED;
      case ECMASCRIPT_NEXT:
        return LanguageMode.ES_NEXT;
      case ECMASCRIPT_NEXT_IN:
        return LanguageMode.ES_NEXT_IN;
      default:
        throw new IllegalStateException("Unexpected language mode: " + options.getLanguageIn());
    }
  }

  @Override
  Config getParserConfig(ConfigContext context) {
    if (parserConfig == null || externsParserConfig == null) {
      synchronized (this) {
        if (parserConfig == null) {
          LanguageMode configLanguageMode = getParserConfigLanguageMode(options.getLanguageIn());
          StrictMode strictMode =
              options.expectStrictModeInput() ? StrictMode.STRICT : StrictMode.SLOPPY;
          parserConfig = createConfig(configLanguageMode, strictMode);
          // Externs must always be parsed with at least ES5 language mode.
          externsParserConfig =
              configLanguageMode.equals(LanguageMode.ECMASCRIPT3)
                  ? createConfig(LanguageMode.ECMASCRIPT5, strictMode)
                  : parserConfig;
        }
      }
    }
    if (context == ConfigContext.EXTERNS) {
      return externsParserConfig;
    }
    return parserConfig;
  }

  protected Config createConfig(LanguageMode mode, StrictMode strictMode) {
    return ParserRunner.createConfig(
        mode,
        options.isParseJsDocDocumentation(),
        options.canContinueAfterErrors() ? RunMode.KEEP_GOING : RunMode.STOP_AFTER_ERROR,
        options.extraAnnotationNames,
        options.parseInlineSourceMaps,
        strictMode);
  }

  // ------------------------------------------------------------------------
  // Error reporting
  // ------------------------------------------------------------------------

  /**
   * The warning classes that are available from the command-line, and are suppressible by the
   * {@code @suppress} annotation.
   */
  public DiagnosticGroups getDiagnosticGroups() {
    return new DiagnosticGroups();
  }

  @Override
  public void report(JSError error) {
    CheckLevel level = error.getDefaultLevel();
    if (warningsGuard != null) {
      CheckLevel newLevel = warningsGuard.level(error);
      if (newLevel != null) {
        level = newLevel;
      }
    }

    if (level.isOn()) {
      initCompilerOptionsIfTesting();
      if (getOptions().errorHandler != null) {
        getOptions().errorHandler.report(level, error);
      }
      errorManager.report(level, error);
    }
  }

  @Override
  public void report(CheckLevel ignoredLevel, JSError error) {
    report(error);
  }

  @Override
  public CheckLevel getErrorLevel(JSError error) {
    checkNotNull(options);
    return warningsGuard.level(error);
  }

  /** Report an internal error. */
  @Override
  void throwInternalError(String message, Throwable cause) {
    throw new RuntimeException(
        "INTERNAL COMPILER ERROR.\nPlease report this problem.\n\n" + message, cause);
  }

  /** Gets the number of errors. */
  public int getErrorCount() {
    return errorManager.getErrorCount();
  }

  /** Gets the number of warnings. */
  public int getWarningCount() {
    return errorManager.getWarningCount();
  }

  @Override
  boolean hasHaltingErrors() {
    return !getOptions().canContinueAfterErrors() && errorManager.hasHaltingErrors();
  }

  /**
   * Consults the {@link ErrorManager} to see if we've encountered errors that should halt
   * compilation.
   *
   * <p>If {@link CompilerOptions#canContinueAfterErrors} is {@code true}, this function always
   * returns {@code false} without consulting the error manager. The error manager will continue to
   * be told about new errors and warnings, but the compiler will complete compilation of all
   * inputs.
   *
   * <p>
   */
  public boolean hasErrors() {
    return hasHaltingErrors();
  }

  @Override
  SourceFile getSourceFileByName(String sourceName) {
    // Here we assume that the source name is the input name, this
    // is true of JavaScript parsed from source.
    if (sourceName != null) {
      CompilerInput input = inputsById.get(new InputId(sourceName));
      if (input != null) {
        return input.getSourceFile();
      }
      // Alternatively, the sourceName might have been reverse-mapped by
      // an input source-map, so let's look in our sourcemap original sources.
      return sourceMapOriginalSources.get(sourceName);
    }

    return null;
  }

  public CharSequence getSourceFileContentByName(String sourceName) {
    SourceFile file = getSourceFileByName(sourceName);
    checkNotNull(file);
    try {
      return file.getCode();
    } catch (IOException e) {
      return null;
    }
  }

  @Override
  public void addInputSourceMap(String sourceFileName, SourceMapInput inputSourceMap) {
    inputSourceMaps.put(sourceFileName, inputSourceMap);
    if (options.sourceMapIncludeSourcesContent && sourceMap != null) {
      addSourceMapSourceFiles(inputSourceMap);
    }
  }

  /**
   * Adds file name to content mappings for all sources found in a source map. This is used to
   * populate sourcesContent array in the output source map even for sources embedded in the input
   * source map.
   */
  private synchronized void addSourceMapSourceFiles(SourceMapInput inputSourceMap) {
    // synchronized annotation guards concurrent access to sourceMap during parsing.
    SourceMapConsumerV3 consumer = inputSourceMap.getSourceMap(errorManager);
    if (consumer == null) {
      return;
    }
    Collection<String> sourcesContent = consumer.getOriginalSourcesContent();
    if (sourcesContent == null) {
      return;
    }
    Iterator<String> content = sourcesContent.iterator();
    Iterator<String> sources = consumer.getOriginalSources().iterator();
    while (sources.hasNext() && content.hasNext()) {
      String code = content.next();
      SourceFile source =
          SourceMapResolver.getRelativePath(inputSourceMap.getOriginalPath(), sources.next());
      if (source != null) {
        sourceMap.addSourceFile(source.getName(), code);
      }
    }
    if (sources.hasNext() || content.hasNext()) {
      throw new RuntimeException(
          "Source map's \"sources\" and \"sourcesContent\" lengths do not match.");
    }
  }

  @Override
  @Nullable
  public OriginalMapping getSourceMapping(String sourceName, int lineNumber, int columnNumber) {
    if (sourceName == null) {
      return null;
    }
    SourceMapInput sourceMap = inputSourceMaps.get(sourceName);
    if (sourceMap == null) {
      return null;
    }

    // JSCompiler uses 1-indexing for lineNumber and 0-indexing for columnNumber.
    // Sourcemaps use 1-indexing for both.
    SourceMapConsumerV3 consumer = sourceMap.getSourceMap(errorManager);
    if (consumer == null) {
      return null;
    }
    OriginalMapping result = consumer.getMappingForLine(lineNumber, columnNumber + 1);
    if (result == null) {
      return null;
    }

    // First check to see if the original file was loaded from an input source map.
    String sourceMapOriginalPath = sourceMap.getOriginalPath();
    String resultOriginalPath = result.getOriginalFile();
    final String relativePath;

    // Resolving the paths to a source file is expensive, so check the cache first.
    if (sourceMapOriginalPath.equals(resolvedSourceMap.originalPath)
        && resultOriginalPath.equals(resolvedSourceMap.sourceMapPath)) {
      relativePath = resolvedSourceMap.relativePath;
    } else {
      relativePath = resolveSibling(sourceMapOriginalPath, resultOriginalPath);
      SourceFile source = getSourceFileByName(relativePath);
      if (source == null && !isNullOrEmpty(resultOriginalPath)) {
        source =
            SourceMapResolver.getRelativePath(
                sourceMap.getOriginalPath(), result.getOriginalFile());
        if (source != null) {
          sourceMapOriginalSources.putIfAbsent(relativePath, source);
        }
      }

      // Cache this resolved source for the next caller.
      resolvedSourceMap.originalPath = sourceMapOriginalPath;
      resolvedSourceMap.sourceMapPath = resultOriginalPath;
      resolvedSourceMap.relativePath = relativePath;
    }

    return result.toBuilder()
        .setOriginalFile(relativePath)
        .setColumnPosition(result.getColumnPosition() - 1)
        .build();
  }

  @Override
  public String getSourceLine(String sourceName, int lineNumber) {
    if (lineNumber < 1) {
      return null;
    }
    SourceFile input = getSourceFileByName(sourceName);
    if (input != null) {
      return input.getLine(lineNumber);
    }
    return null;
  }

  @Override
  public Region getSourceLines(String sourceName, int lineNumber, int length) {
    if (lineNumber < 1) {
      return null;
    }
    SourceFile input = getSourceFileByName(sourceName);
    if (input != null) {
      return input.getLines(lineNumber, length);
    }
    return null;
  }

  @Override
  public Region getSourceRegion(String sourceName, int lineNumber) {
    if (lineNumber < 1) {
      return null;
    }
    SourceFile input = getSourceFileByName(sourceName);
    if (input != null) {
      return input.getRegion(lineNumber);
    }
    return null;
  }

  // ------------------------------------------------------------------------
  // Package-private and Protected helpers
  // ------------------------------------------------------------------------

  @Override
  protected Node getNodeForCodeInsertion(@Nullable JSChunk module) {
    if (this.inputsById.containsKey(SYNTHETIC_CODE_INPUT_ID)) {
      return this.inputsById.get(SYNTHETIC_CODE_INPUT_ID).getAstRoot(this);
    }
    if (module == null) {
      if (moduleGraph == null || Iterables.isEmpty(moduleGraph.getAllInputs())) {
        throw new IllegalStateException("No inputs");
      }
      CompilerInput firstInput = Iterables.getFirst(moduleGraph.getAllInputs(), null);
      // TODO(lharker): add `checkNotModule(`.
      return firstInput.getAstRoot(this);
    }

    List<CompilerInput> moduleInputs = module.getInputs();
    if (!moduleInputs.isEmpty()) {
      return checkNotModule(
          moduleInputs.get(0).getAstRoot(this), "Cannot insert code into a module");
    }
    throw new IllegalStateException("Root module has no inputs");
  }

  public SourceMap getSourceMap() {
    return sourceMap;
  }

  /** Ids for cross-module method stubbing, so that each method has a unique id. */
  private IdGenerator crossModuleIdGenerator = new IdGenerator();

  /**
   * Keys are arguments passed to getCssName() found during compilation; values are the number of
   * times the key appeared as an argument to getCssName().
   */
  private Map<String, Integer> cssNames = null;

  /** The variable renaming map */
  private VariableMap variableMap = null;

  /** The property renaming map */
  private VariableMap propertyMap = null;

  /** String replacement map */
  private VariableMap stringMap = null;

  /** Mapping for Instrumentation parameter encoding */
  private VariableMap instrumentationMapping = null;

  /** Id generator map */
  private String idGeneratorMap = null;

  /** Names exported by goog.exportSymbol. */
  private final Set<String> exportedNames = new LinkedHashSet<>();

  @Override
  public void setVariableMap(VariableMap variableMap) {
    this.variableMap = variableMap;
  }

  VariableMap getVariableMap() {
    return variableMap;
  }

  @Override
  public void setPropertyMap(VariableMap propertyMap) {
    this.propertyMap = propertyMap;
  }

  VariableMap getPropertyMap() {
    return this.propertyMap;
  }

  @Override
  public void setStringMap(VariableMap stringMap) {
    this.stringMap = stringMap;
  }

  @Override
  public void setCssNames(Map<String, Integer> cssNames) {
    this.cssNames = cssNames;
  }

  @Override
  public void setIdGeneratorMap(String serializedIdMappings) {
    this.idGeneratorMap = serializedIdMappings;
  }

  @Override
  public IdGenerator getCrossModuleIdGenerator() {
    return crossModuleIdGenerator;
  }

  @Override
  public void setAnonymousFunctionNameMap(VariableMap functionMap) {
    // NOTE: remove this method
  }

  VariableMap getStringMap() {
    return this.stringMap;
  }

  @Override
  public void setInstrumentationMapping(VariableMap instrumentationMapping) {
    this.instrumentationMapping = instrumentationMapping;
  }

  public VariableMap getInstrumentationMapping() {
    return this.instrumentationMapping;
  }

  @Override
  public void addExportedNames(Set<String> exportedNames) {
    this.exportedNames.addAll(exportedNames);
  }

  @Override
  public Set<String> getExportedNames() {
    return exportedNames;
  }

  @Override
  public CompilerOptions getOptions() {
    return options;
  }

  /** Sets the logging level for the com.google.javascript.jscomp package. */
  public static void setLoggingLevel(Level level) {
    logger.setLevel(level);
  }

  /** Gets the DOT graph of the AST generated at the end of compilation. */
  public String getAstDotGraph() throws IOException {
    if (jsRoot != null) {
      ControlFlowAnalysis cfa = new ControlFlowAnalysis(this, true, false);
      cfa.process(null, jsRoot);
      return DotFormatter.toDot(jsRoot, cfa.getCfg());
    } else {
      return "";
    }
  }

  @Override
  public ErrorManager getErrorManager() {
    if (options == null) {
      initOptions(new CompilerOptions());
    }
    return errorManager;
  }

  @Override
  Iterable<CompilerInput> getInputsInOrder() {
    return moduleGraph.getAllInputs();
  }

  @Override
  int getNumberOfInputs() {
    // In some testing cases inputs will be null, but obviously there must be at least one input.
    // The intended use of this method is to allow passes to estimate how much memory they will
    // need for data structures, so it's not necessary that the returned value be exactly right
    // in the corner cases where inputs ends up being null.
    return (moduleGraph != null) ? moduleGraph.getInputCount() : 1;
  }

  /** Returns an unmodifiable view of the compiler inputs indexed by id. */
  public Map<InputId, CompilerInput> getInputsById() {
    return Collections.unmodifiableMap(inputsById);
  }

  /** Gets the externs in the order in which they are being processed. */
  List<CompilerInput> getExternsInOrder() {
    return Collections.unmodifiableList(externs);
  }

  @VisibleForTesting
  List<CompilerInput> getInputsForTesting() {
    return moduleGraph != null ? ImmutableList.copyOf(moduleGraph.getAllInputs()) : null;
  }

  @VisibleForTesting
  List<CompilerInput> getExternsForTesting() {
    return externs;
  }

  @Override
  boolean hasRegExpGlobalReferences() {
    return hasRegExpGlobalReferences;
  }

  @Override
  void setHasRegExpGlobalReferences(boolean references) {
    hasRegExpGlobalReferences = references;
  }

  /** Set whether J2CL passes should run */
  @Override
  void setRunJ2clPasses(boolean runJ2clPasses) {
    this.runJ2clPasses = runJ2clPasses;
  }

  /** Whether J2CL passes should run */
  @Override
  boolean runJ2clPasses() {
    return runJ2clPasses;
  }

  @Override
  CompilerInput getSynthesizedExternsInput() {
    if (syntheticExternsInput != null) {
      return syntheticExternsInput;
    }

    CompilerInput input = new CompilerInput(SYNTHETIC_EXTERNS_FILE, /* isExtern= */ true);
    Node root = checkNotNull(input.getAstRoot(this));
    putCompilerInput(input);
    this.syntheticExternsInput = input;
    externsRoot.addChildToFront(root);
    externs.add(0, input);
    scriptNodeByFilename.put(input.getSourceFile().getName(), root);

    return input;
  }

  @Override
  InputId getSyntheticCodeInputId() {
    return SYNTHETIC_CODE_INPUT_ID;
  }

  @Override
  void initializeSyntheticCodeInput() {
    checkState(
        !this.inputsById.containsKey(SYNTHETIC_CODE_INPUT_ID),
        "Already initialized synthetic input");
    JsAst ast = new JsAst(SourceFile.fromCode(SYNTHETIC_CODE_INPUT_ID.getIdName(), ""));
    if (inputsById.containsKey(ast.getInputId())) {
      throw new IllegalStateException("Conflicting synthetic id name");
    }
    CompilerInput input = new CompilerInput(ast, false);
    jsRoot.addChildToFront(checkNotNull(ast.getAstRoot(this)));

    JSChunk firstModule = Iterables.getFirst(getModules(), null);
    if (firstModule.getName().equals(JSChunk.STRONG_CHUNK_NAME)) {
      firstModule.add(input);
    }
    input.setModule(firstModule);
    putCompilerInput(input);

    commentsPerFile.put(SYNTHETIC_CODE_INPUT_ID.getIdName(), ImmutableList.of());
    reportChangeToChangeScope(ast.getAstRoot(this));
  }

  @Override
  void removeSyntheticCodeInput() {
    this.removeSyntheticCodeInput(/* mergeContentIntoFirstInput= */ false);
  }

  @Override
  void mergeSyntheticCodeInput() {
    this.removeSyntheticCodeInput(/* mergeContentIntoFirstInput= */ true);
  }

  @Override
  void setLocaleSubstitutionData(LocaleData localeDataValueMap) {
    this.localeDataValueMap = localeDataValueMap;
  }

  @Override
  LocaleData getLocaleSubstitutionData() {
    return localeDataValueMap;
  }

  /**
   * Deletes the synthesized code input and optionally moves all its subtree contents into the first
   * non-synthetic input
   */
  private void removeSyntheticCodeInput(boolean mergeContentIntoFirstInput) {
    checkState(
        this.inputsById.containsKey(SYNTHETIC_CODE_INPUT_ID),
        "Never initialized the synthetic input");
    CompilerInput input = this.inputsById.get(SYNTHETIC_CODE_INPUT_ID);
    Node astRoot = input.getAstRoot(this);
    checkState(astRoot.isFirstChildOf(jsRoot));
    checkState(SYNTHETIC_CODE_INPUT_ID.equals(input.getInputId()));

    if (mergeContentIntoFirstInput && astRoot.hasChildren()) {
      // If we've inserted anything into the synthetic AST, move it into the top of the next script.
      Node next = astRoot.getNext();
      checkNotNull(next, "Must provide at least one source");
      checkNotModule(
          next, "Cannot remove synthetic code input until modules are rewritten: %s", next);
      next.addChildrenToFront(astRoot.removeChildren());
      reportChangeToChangeScope(next);
    }

    astRoot.detach();

    // bookkeeping to mark scopes and nodes as deleted
    reportChangeToChangeScope(astRoot);
    astRoot.setDeleted(true);
    NodeUtil.markFunctionsDeleted(astRoot, this);

    input.getChunk().remove(input);
    inputsById.remove(input.getInputId());
  }

  @Override
  public double getProgress() {
    return progress;
  }

  @Override
  String getLastPassName() {
    return lastPassName;
  }

  @Override
  void setProgress(double newProgress, String passName) {
    this.lastPassName = passName;
    progress = min(newProgress, 1.0);
  }

  @Override
  void setExternProperties(ImmutableSet<String> externProperties) {
    this.externProperties = externProperties;
  }

  @Override
  public ImmutableSet<String> getExternProperties() {
    return externProperties;
  }

  @Override
  AccessorSummary getAccessorSummary() {
    return accessorSummary;
  }

  @Override
  public void setAccessorSummary(AccessorSummary summary) {
    this.accessorSummary = checkNotNull(summary);
  }

  @Override
  protected Node ensureLibraryInjected(String resourceName, boolean force) {
    boolean shouldInject =
        force || (!options.skipNonTranspilationPasses && !options.preventLibraryInjection);
    if (injectedLibraries.contains(resourceName) || !shouldInject) {
      return lastInjectedLibrary;
    }

    checkState(!getLifeCycleStage().isNormalized(), "runtime library injected after normalization");

    // Load/parse the code.
    String path = String.join("", AbstractCompiler.RUNTIME_LIB_DIR, resourceName, ".js");
    final Node ast;
    if (this.getLifeCycleStage().hasColorAndSimplifiedJSDoc()) {
      checkNotNull(
          this.runtimeLibraryTypedAsts,
          "Must call initRuntimeLibraryTypedAsts before calling ensureLibraryInjected during"
              + " optimizations");
      ast = this.runtimeLibraryTypedAsts.get(path).get();
    } else {
      checkState(
          !this.hasTypeCheckingRun(),
          "runtime library injected after type checking but before optimization colors");
      String originalCode =
          ResourceLoader.loadTextResource(Compiler.class, "js/" + resourceName + ".js");

      SourceFile source = SourceFile.fromCode(path, originalCode);
      addFilesToSourceMap(ImmutableList.of(source));
      ast = parseCodeHelper(source);
    }

    // Look for string literals of the form 'require foo bar' or 'declare baz''.
    // As we process each one, remove it from its parent.
    for (Node node = ast.getFirstChild();
        node != null && node.isExprResult() && node.getFirstChild().isStringLit();
        node = ast.getFirstChild()) {
      String directive = node.getFirstChild().getString();
      List<String> words = Splitter.on(' ').limit(2).splitToList(directive);
      switch (words.get(0)) {
        case "use":
          // 'use strict' is ignored (and deleted).
          break;
        case "require":
          // 'require lib'; pulls in the named library before this one.
          ensureLibraryInjected(words.get(1), force);
          break;
        default:
          throw new RuntimeException("Bad directive: " + directive);
      }
      node.detach();
    }

    // Insert the code immediately after the last-inserted runtime library.
    Node lastChild = ast.getLastChild();
    for (Node child = ast.getFirstChild(); child != null; child = child.getNext()) {
      NodeUtil.markNewScopesChanged(child, this);
    }
    Node firstChild = ast.removeChildren();
    if (firstChild == null) {
      // Handle require-only libraries.
      return lastInjectedLibrary;
    }
    Node parent = getNodeForCodeInsertion(null);
    if (lastInjectedLibrary == null) {
      parent.addChildrenToFront(firstChild);
    } else {
      parent.addChildrenAfter(firstChild, lastInjectedLibrary);
    }
    lastInjectedLibrary = lastChild;
    injectedLibraries.add(resourceName);

    reportChangeToEnclosingScope(parent);
    return lastChild;
  }

  @Override
  void addComments(String filename, List<Comment> comments) {
    if (!getOptions().preservesDetailedSourceInfo()) {
      throw new UnsupportedOperationException("addComments may only be called in IDE mode.");
    }
    commentsPerFile.put(filename, comments);
  }

  @Override
  public List<Comment> getComments(String filename) {
    if (!getOptions().preservesDetailedSourceInfo()) {
      throw new UnsupportedOperationException("getComments may only be called in IDE mode.");
    }
    return commentsPerFile.get(filename);
  }

  @Override
  public ModuleLoader getModuleLoader() {
    return moduleLoader;
  }

  private synchronized void addFilesToSourceMap(Iterable<SourceFile> files) {
    // synchronized annotation guards concurrent access to sourceMap during parsing.
    if (getOptions().sourceMapIncludeSourcesContent && getSourceMap() != null) {
      for (SourceFile file : files) {
        try {
          getSourceMap().addSourceFile(file.getName(), file.getCode());
        } catch (IOException e) {
          throw new RuntimeException("Cannot read code of a source map's source file.", e);
        }
      }
    }
  }

  public void initWebpackMap(ImmutableMap<String, String> inputPathByWebpackId) {
    this.inputPathByWebpackId = inputPathByWebpackId;
  }

  protected CompilerExecutor createCompilerExecutor() {
    return new CompilerExecutor();
  }

  protected CompilerExecutor getCompilerExecutor() {
    return compilerExecutor;
  }

  /**
   * Serializable state of the compiler specific to multistage binary builds
   *
   * <p>Only contains state that does not make sense in 'multilevel' binary builds (where
   * library-level TypedASTs are the input). Such state belongs in the jscomp.TypedAst proto.
   */
  private static class CompilerState implements Serializable {

    private final FeatureSet featureSet;
    private final boolean typeCheckingHasRun;
    private final boolean hasRegExpGlobalReferences;
    private final LifeCycleStage lifeCycleStage;
    private final JSChunkGraph moduleGraph;
    private final int uniqueNameId;
    private final UniqueIdSupplier uniqueIdSupplier;
    private final Set<String> exportedNames;
    private final Map<String, Integer> cssNames;
    private final String idGeneratorMap;
    private final IdGenerator crossModuleIdGenerator;
    private final boolean runJ2clPasses;
    private final ConcurrentHashMap<String, SourceMapInput> inputSourceMaps;
    private final int changeStamp;
    private final ImmutableList<InputId> externs;
    private final ImmutableListMultimap<JSChunk, InputId> moduleToInputList;
    private final LinkedHashSet<String> injectedLibraries;
    private final int lastInjectedLibraryIndexInFirstScript;
    private final AccessorSummary accessorSummary;

    CompilerState(Compiler compiler) {
      this.featureSet = checkNotNull(compiler.featureSet);
      this.typeCheckingHasRun = compiler.typeCheckingHasRun;
      this.hasRegExpGlobalReferences = compiler.hasRegExpGlobalReferences;
      this.lifeCycleStage = compiler.getLifeCycleStage();
      this.moduleGraph = compiler.moduleGraph;
      this.uniqueNameId = compiler.uniqueNameId;
      this.uniqueIdSupplier = compiler.uniqueIdSupplier;
      this.exportedNames = compiler.exportedNames;
      this.cssNames = compiler.cssNames;
      this.idGeneratorMap = compiler.idGeneratorMap;
      this.crossModuleIdGenerator = compiler.crossModuleIdGenerator;
      this.runJ2clPasses = compiler.runJ2clPasses;
      this.inputSourceMaps = compiler.inputSourceMaps;
      this.changeStamp = compiler.changeStamp;
      this.externs =
          compiler.externs.stream().map(CompilerInput::getInputId).collect(toImmutableList());
      this.moduleToInputList = mapJSModulesToInputIds(compiler.moduleGraph.getAllChunks());
      this.injectedLibraries = compiler.injectedLibraries;
      this.lastInjectedLibraryIndexInFirstScript =
          compiler.lastInjectedLibrary != null
              ? compiler.jsRoot.getFirstChild().getIndexOfChild(compiler.lastInjectedLibrary)
              : -1;
      this.accessorSummary = compiler.accessorSummary;
    }
  }

  private static final ImmutableListMultimap<JSChunk, InputId> mapJSModulesToInputIds(
      Iterable<JSChunk> jsModules) {
    ImmutableListMultimap.Builder<JSChunk, InputId> jsmoduleToInputId =
        ImmutableListMultimap.builder();
    for (JSChunk jsModule : jsModules) {
      jsmoduleToInputId.putAll(
          jsModule,
          jsModule.getInputs().stream().map(CompilerInput::getInputId).collect(toImmutableList()));
    }
    return jsmoduleToInputId.build();
  }

  @GwtIncompatible("ObjectOutputStream")
  public void saveState(OutputStream outputStream) throws IOException {
    // Do not close the outputstream, caller is responsible for closing it.
    runInCompilerThread(
        () -> {
          Tracer tracer = newTracer("serializeCompilerState");
          new ObjectOutputStream(outputStream).writeObject(new CompilerState(this));
          stopTracer(tracer, "serializeCompilerState");
          tracer = newTracer("serializeTypedAst");
          LocaleDataPasses.addLocaleDataToAST(this, this.getLocaleSubstitutionData());
          SerializeTypedAstPass.createFromOutputStream(this, outputStream)
              .process(externsRoot, jsRoot);
          stopTracer(tracer, "serializeTypedAst");
          return null;
        });
  }

  @GwtIncompatible("ClassNotFoundException")
  public void restoreState(InputStream inputStream) throws IOException, ClassNotFoundException {
    initWarningsGuard(options.getWarningsGuard());
    maybeSetTracker();

    runInCompilerThread(
        () -> {
          Tracer tracer = newTracer(PassNames.DESERIALIZE_COMPILER_STATE);
          logger.fine("Deserializing the CompilerState");
          try {
            deserializeCompilerState(inputStream);
            return null;
          } finally {
            logger.fine("Finished deserializing CompilerState");
            stopTracer(tracer, PassNames.DESERIALIZE_COMPILER_STATE);
          }
        });

    if (tracker != null) {
      tracker.updateAfterDeserialize(jsRoot);
    }
  }

  @GwtIncompatible("ObjectInputStream")
  // this method must be called from within a "compiler thread" with a larger stack
  private void deserializeCompilerState(InputStream inputStream)
      throws IOException, ClassNotFoundException {
    // Do not close the input stream, caller is responsible for closing it.
    CompilerState compilerState = (CompilerState) new ObjectInputStream(inputStream).readObject();

    checkNotNull(
        this.moduleGraph, "Did you forget to call .init or .initModules before restoreState?");
    ImmutableMap.Builder<String, SourceFile> externFilesBuilder = ImmutableMap.builder();
    ImmutableMap.Builder<String, SourceFile> codeFilesBuilder = ImmutableMap.builder();
    ImmutableList.Builder<SourceFile> allInputFiles = ImmutableList.builder();
    for (CompilerInput input : this.moduleGraph.getAllInputs()) {
      allInputFiles.add(input.getSourceFile());
      codeFilesBuilder.put(input.getInputId().getIdName(), input.getSourceFile());
    }
    for (CompilerInput extern : this.externs) {
      allInputFiles.add(extern.getSourceFile());
      externFilesBuilder.put(extern.getInputId().getIdName(), extern.getSourceFile());
    }

    TypedAstDeserializer.DeserializedAst deserializedAst =
        TypedAstDeserializer.deserializeFullAst(
            this,
            SYNTHETIC_EXTERNS_FILE,
            allInputFiles.build(),
            inputStream,
            compilerState.typeCheckingHasRun);

    featureSet = compilerState.featureSet;
    scriptNodeByFilename.clear();
    typeCheckingHasRun = compilerState.typeCheckingHasRun;
    injectedLibraries.clear();
    injectedLibraries.addAll(compilerState.injectedLibraries);
    hasRegExpGlobalReferences = compilerState.hasRegExpGlobalReferences;
    // after restoreState, we're always guaranteed to have colors & simplified JSDoc on the AST.
    // whether the AST is also normalized depends on when saveState was called (after stage 1 or 2)
    LifeCycleStage stage =
        compilerState.lifeCycleStage == LifeCycleStage.RAW
            ? LifeCycleStage.COLORS_AND_SIMPLIFIED_JSDOC
            : compilerState.lifeCycleStage;
    setLifeCycleStage(stage);
    moduleGraph = compilerState.moduleGraph;
    uniqueNameId = compilerState.uniqueNameId;
    uniqueIdSupplier = compilerState.uniqueIdSupplier;
    exportedNames.clear();
    exportedNames.addAll(compilerState.exportedNames);
    cssNames = compilerState.cssNames;
    variableMap = null;
    propertyMap = null;
    stringMap = null;
    idGeneratorMap = compilerState.idGeneratorMap;
    crossModuleIdGenerator = compilerState.crossModuleIdGenerator;
    runJ2clPasses = compilerState.runJ2clPasses;
    inputSourceMaps = compilerState.inputSourceMaps;
    changeStamp = compilerState.changeStamp;
    accessorSummary = compilerState.accessorSummary;

    // Restore TypedAST and related fields
    externProperties = deserializedAst.getExternProperties();
    externAndJsRoot = IR.root(IR.root(), IR.root());
    externsRoot = externAndJsRoot.getFirstChild();
    jsRoot = externAndJsRoot.getLastChild();
    inputsById.clear();
    externs.clear();

    colorRegistry = deserializedAst.getColorRegistry().orNull();

    // Tells CompilerInput::getRoot to deserialize an AST rather than re-parsing the file
    this.typedAstFilesystem = deserializedAst.getFilesystem();

    // overwrite any existing CompilerInput instances. Reuse the SourceFiles created by
    // TypedAstDeserializer; otherwise lookups in this.typedAstFilesystem will fail.
    ImmutableMap<String, SourceFile> externFiles = externFilesBuilder.buildOrThrow();
    ImmutableMap<String, SourceFile> codeFiles = codeFilesBuilder.buildOrThrow();
    for (InputId extern : compilerState.externs) {
      if (extern.getIdName().equals(SYNTHETIC_EXTERNS_FILE.getName())) {
        this.getSynthesizedExternsInput();
        continue;
      }
      SourceFile externFile = externFiles.get(extern.getIdName());
      if (externFile == null) {
        // Extern files may passed in with regular source files, and later 'hoisted' to externs
        // because they are annotated `@externs`.
        externFile = checkNotNull(codeFiles.get(extern.getIdName()), "Missing %s", extern);
      }
      CompilerInput input = new CompilerInput(externFile, /* isExtern= */ true);
      Node script = input.getAstRoot(this); // accesses this.typedAstFilesystem

      externsRoot.addChildToBack(script);
      inputsById.put(script.getInputId(), input);
      scriptNodeByFilename.put(externFile.getName(), script);
      this.externs.add(input);
    }

    for (JSChunk deserializedModule : getModules()) {
      for (InputId inputId : compilerState.moduleToInputList.get(deserializedModule)) {
        SourceFile src = checkNotNull(codeFiles.get(inputId.getIdName()), "Missing %s", inputId);
        CompilerInput input = new CompilerInput(src);
        Node script = input.getAstRoot(this); // accesses this.typedAstFilesystem

        jsRoot.addChildToBack(script);
        scriptNodeByFilename.put(src.getName(), script);

        putCompilerInput(input); // overwrite the old input
        deserializedModule.add(input);
      }
    }

    this.typedAstFilesystem = null; // allow garbage collection

    setLocaleSubstitutionData(LocaleDataPasses.reconstituteLocaleDataFromAST(this));
    lastInjectedLibrary =
        compilerState.lastInjectedLibraryIndexInFirstScript != -1
            ? jsRoot
                .getFirstChild()
                .getChildAtIndex(compilerState.lastInjectedLibraryIndexInFirstScript)
            : null;
  }

  /** Returns the module type for the provided namespace. */
  @Override
  @Nullable
  ModuleType getModuleTypeByName(String moduleName) {
    return moduleTypesByName.get(moduleName);
  }

  private ModuleMetadataMap moduleMetadataMap;

  @Override
  public ModuleMetadataMap getModuleMetadataMap() {
    return moduleMetadataMap;
  }

  @Override
  public void setModuleMetadataMap(ModuleMetadataMap moduleMetadataMap) {
    this.moduleMetadataMap = moduleMetadataMap;
  }

  private ModuleMap moduleMap;

  @Override
  public ModuleMap getModuleMap() {
    return moduleMap;
  }

  @Override
  public void setModuleMap(ModuleMap moduleMap) {
    this.moduleMap = moduleMap;
  }

  /**
   * Simplistic implementation of the java.nio.file.Path resolveSibling method that always uses "/"
   * regardless of platform. This is required for consistency of path handling when using input
   * source maps, which are expected to always use "/".
   *
   * <p>If `toPath` is already absolute, just return it.
   *
   * <p>Otherwise, strip the filename off of `fromPath` and logically replaces it with `toPath`.
   * Then normalize the path by resolving and removing any "." or ".." elements.
   *
   * @param fromPath - must be a file (not directory)
   * @param toPath - must be a file (not directory)
   * @return `toPath` adjusted so that it has the same parent directory as `fromPath`
   */
  private static String resolveSibling(String fromPath, String toPath) {
    // If the destination is an absolute path, nothing to do.
    if (toPath.startsWith("/")) {
      return toPath;
    }

    List<String> fromPathParts = new ArrayList<>(Arrays.asList(fromPath.split("/")));
    List<String> toPathParts = new ArrayList<>(Arrays.asList(toPath.split("/")));
    if (!fromPathParts.isEmpty()) {
      fromPathParts.remove(fromPathParts.size() - 1);
    }

    while (!fromPathParts.isEmpty() && !toPathParts.isEmpty()) {
      if (toPathParts.get(0).equals(".")) {
        toPathParts.remove(0);
      } else if (toPathParts.get(0).equals("..")) {
        toPathParts.remove(0);
        fromPathParts.remove(fromPathParts.size() - 1);
      } else {
        break;
      }
    }

    fromPathParts.addAll(toPathParts);
    return String.join("/", fromPathParts);
  }

  public void resetAndIntitializeSourceMap() {
    if (sourceMap == null) {
      return;
    }
    sourceMap.reset();
    if (options.sourceMapIncludeSourcesContent) {
      if (options.applyInputSourceMaps) {
        // Add any input source map content files to the source map as potential sources
        for (SourceMapInput inputSourceMap : inputSourceMaps.values()) {
          addSourceMapSourceFiles(inputSourceMap);
        }
      }

      // Add all the compilation sources to the source map as potential sources
      Iterable<JSChunk> allModules = getModules();
      if (allModules != null) {
        List<SourceFile> sourceFiles = new ArrayList<>();
        for (JSChunk module : allModules) {
          for (CompilerInput input : module.getInputs()) {
            sourceFiles.add(input.getSourceFile());
          }
        }
        addFilesToSourceMap(sourceFiles);
      }
    }
  }

  private static Node checkNotModule(Node script, String msg, Object... args) {
    checkArgument(script.isScript(), script);
    if (!script.hasOneChild()) {
      return script;
    }
    checkState(!script.getFirstChild().isModuleBody(), msg, args);
    return script;
  }
}
