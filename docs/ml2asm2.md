




Pasted text.txt
Document

ml2asm.md
File
I want to write an OS on embedded cor24 (plus I2C SPI I/O), using primarily my own Macro Lisp (compiled to .s).  I want it to be flexible with defining/calling .s subroutines, maybe access registers, memory mapped I/O, etc.  However I do not want to reinvent the wheel, but find a precedent to follow.  Here's what my AI coding agent researched, but I want more Pros/Cons before deciding.  Also, background, I have a Mac/Linux-hosted C compiler to build my macro lisp into .s assembler.  First pass is to cross-compile to produce .s on the target cor24 hw (or emulator).  Later passes are to assemble on-the-fly within macro lisp so that could code be developed on the cor24 hw/emulator with no Mac/Linux/Rust.

---


Thought

Thought for 19s
You want an embedded COR24 OS whose primary systems language is your Macro Lisp, with a clean path from:

hosted cross-compilation to .s now,

mixed Lisp + native assembly soon,

eventual on-target self-hosting later.

You also want good precedents, not just “it works,” and you want the tradeoffs before you commit.

My recommendation
Use a layered design, but do not make raw string-based (asm ...) the long-term center of the system.

Use this progression:

Near term: keep (asm ...) only as a bootstrap escape hatch for cross-compiling.

Main design: define a small set of compiler-known primops / machine primitives for the things the OS really needs.

Next layer: add named native routines with a stable ABI, callable from Lisp.

Later: add a Lisp-level assembler or LAP-like DSL for on-target development.

That is very close to the most successful historical precedents:

Lisp Machines leaned on compiler/runtime-known low-level primitives instead of making application code be raw assembly strings. 

SBCL backend work is structured around machine-aware templates/VOPs and assembler support, not user-facing opaque asm strings. 

CHICKEN Scheme is a closer precedent for your current bootstrap phase: a higher-level language that targets another low-level language and offers controlled foreign escape hatches. 

Clojure and ClojureScript mostly expose the host platform through structured interop, not assembly-like raw insertion into the language core. 

That aligns pretty well with the three-stage path in your uploaded design note: cross-compiled inline asm first, runtime native calling second, Lisp assembler third. 

 

 


The core architectural question
The real decision is not “should Lisp call assembly?”

That answer is clearly yes for an embedded OS.

The real question is:

What should be the primary low-level interface exposed to normal Lisp code?

You have four plausible choices.

Option A: raw inline (asm ...) strings
This is the simplest thing to bootstrap, and your agent already pushed in that direction. Your design note describes exactly that: add asm as a special form that emits literal assembly lines into compiler output with no parsing or validation. 


Pros
Fastest path to first useful OS code

Great for LED, UART, MMIO, ISR stubs, startup code.

No new assembler/parser needed

Reuses your existing assembler.

Perfect for cross-compilation

Fits your current workflow.

Useful as an emergency escape hatch forever

There will always be a few places where dropping to native code is the right answer.

Cons
Opaque to the compiler

No register liveness info, no stack safety, no clobber tracking.

Easy to create ABI drift

One bad block that trashes fp, sp, or a saved register can destabilize the whole system.

Poor abstraction boundary

MMIO policy, interrupt masking, calling convention tricks, and register protocol start leaking everywhere.

Hard to refactor

You cannot reason about or transform string code.

Bad for long-term self-hosting

Strings are okay as a temporary bridge, weak as the canonical internal representation.

My verdict
Do it, but keep it narrow.
Use it for:

bootstrap runtime,

startup/exception/interrupt glue,

a few performance-critical leaf routines,

temporary experiments.

Do not let it become the standard way device drivers and kernel services are written.

Option B: compiler-known machine primitives
This is the approach I would make the main precedent for your OS.

Think of forms like:

(mmio-read24 addr)
(mmio-write24 addr val)
(reg-load 'r0)
(reg-store 'r1 x)
(disable-interrupts)
(enable-interrupts)
(jsr-native sym args...)
or slightly more abstract:

(uart-putc ch)
(spi-xfer ctl tx rx n)
(i2c-start bus)
(i2c-write-byte bus b)
(i2c-read-byte bus ack?)
These compile to known assembly sequences or calls to known runtime stubs.

Why this is historically stronger
This is much closer to the Lisp Machine idea of exposing low-level capability as known subprimitives, and to the SBCL/VOP style where the compiler understands the machine-level operation instead of treating it as a string blob. 

Pros
Compiler can reason about them

Even if your compiler is simple now, this gives you room later.

Much safer

You decide which registers are inputs, outputs, and preserved.

Better ergonomics

Driver code stays Lisp-ish.

Portable across compiler phases

Same source can later target host-generated .s, on-target assembler, or even emulator intrinsics.

Easier to test

You can mock these in the emulator or trace them.

Cons
More front-end/compiler work now

You need a real low-level interface design instead of punting to strings.

May feel restrictive early

You will occasionally need a new primitive before the compiler knows about it.

My verdict
Best long-term center of gravity.

If you only adopt one principle, adopt this one:

Normal OS code should speak in primops and named runtime entry points, not in raw assembly text.

Option C: named native routines plus call-native
Your design note proposes exactly this as phase 2: Lisp calls machine code at a known address via a runtime bridge that respects the COR24 ABI. 


That is a very good idea.

Pros
Clear ABI boundary

Lisp side and assembly side meet in one disciplined place.

Great for drivers and kernel internals

Write hand-tuned routines in .s, expose them by stable names.

Better than inline asm for reuse

One native routine can serve many callers.

Easier migration path

First cross-compiled, later assembled on target.

Cons
You need a trampoline / bridge

This is the hardest part technically.

Argument marshalling matters

Fixnums, pointers, tagged values, stack layout, return conventions.

Potentially awkward for tiny one-off sequences

For three instructions, a full native stub can feel heavy.

My verdict
This should be your main low-level ABI mechanism.

Use it for:

scheduler/context-switch hooks,

ISR handlers and dispatch helpers,

SPI/I2C transfer kernels,

optimized memory ops,

boot/runtime services,

machine-dependent code.

In other words:

(asm ...) for the exceptional case,
call-native/named stubs for the standard case.

Option D: a Lisp assembler / LAP-style DSL
Your phase 3 proposes on-target assembly from Lisp symbolic forms. 


This is the most “Lispy” end state, and historically it has excellent precedent. Systems in the Lisp family have often used assembler DSLs or macro assemblers instead of embedding raw text. SBCL backend work itself uses Lisp macros and assembler machinery to generate native code. 

Pros
Best self-hosting story

Structured, macro-friendly, transformable

Supports labels, relocations, templates cleanly

Lets Lisp generate machine code using Lisp abstractions

Cons
Higher implementation cost

Needs label resolution, encoding, diagnostics, macro conventions

Can distract from finishing the OS

My verdict
Excellent destination, wrong first milestone.

Build toward it, but do not wait for it before writing useful OS pieces.

What other Lisp families suggest
Common Lisp
Portable Common Lisp does not standardize inline assembly. Implementations expose low-level access in implementation-specific ways. In practice, serious low-level work lives in backend machinery, FFI, runtime support, or implementation-specific assembler facilities rather than in portable CL source. SBCL’s backend architecture is strongly in this camp. 

Takeaway:
CL precedent favors structured implementation-specific low-level hooks, not “user code sprays asm strings.”

Clojure
Clojure uses host interop with Java. It does not try to be an assembler for the JVM; it calls into the host platform with structured forms. 

Takeaway:
Expose the host/machine as interop, not as raw textual injection.

ClojureScript
ClojureScript has js*, which is a direct low-level escape hatch into emitted JavaScript, but the normal and preferred path is still structured JS interop forms. 

Takeaway:
A raw escape hatch is useful, but it should not be the main abstraction.

CHICKEN / Gambit style Scheme
These are good precedents for your current bootstrap phase: they target C and expose foreign/code-generation escape hatches because they are compilers that lower into another language/runtime. CHICKEN’s foreign interface is explicitly source-based because CHICKEN translates Scheme to C. 

Takeaway:
Your current (asm ...) idea is reasonable as a transitional source-level escape hatch.

Lisp Machines
The strongest high-level precedent for your OS goal is not “inline asm everywhere,” but a language-aware operating environment with machine-specific primitives and runtime support. That is the spirit worth copying.

Takeaway:
For an OS written “primarily in Lisp,” you want:

a small privileged low-level substrate,

machine-aware primitives,

most of the system above that in Lisp.

My recommended design
1. Keep three layers, but rename their roles mentally
Layer 0 — machine substrate
Handwritten .s:

reset/startup,

trap/interrupt entry,

context switch,

stack switching,

raw SPI/I2C/UART helpers,

memory copy/fill,

native call trampoline,

GC/runtime-critical glue if needed.

This should be small and explicit.

Layer 1 — Lisp-visible primops
Compiler-known or runtime-known operations:

mmio-read8/16/24

mmio-write8/16/24

irq-disable, irq-enable

native-call

port-in, port-out

maybe with-interrupts-disabled

maybe peek-word, poke-word, block-copy

maybe register access only if truly needed

These are the real contract between Lisp and machine.

Layer 2 — OS services in Macro Lisp
scheduler,

message passing,

device protocols,

file abstractions,

buffer management,

REPL/editor integration,

shell/monitor,

higher-level drivers.

This is where you want most code to live.

2. Do not expose arbitrary register mutation too early
You mentioned “maybe access registers.”

That is tempting, but risky.

Good idea
Expose register access:

inside assembler DSL,

inside special trusted runtime modules,

maybe in debugger/monitor code.

Bad idea
Let arbitrary application-level Lisp casually treat machine registers as general variables.

Why?

It couples source code to codegen details.

It breaks future compiler evolution.

It makes ordinary Lisp code non-compositional.

Better pattern:

expose ABI-defined native entry points,

expose MMIO ops,

expose structured primops,

keep register twiddling inside native stubs or assembler DSL.

3. Prefer named native services over ad hoc inline asm
Instead of this style everywhere:

(asm "la r0,#xFF0100"
     "sb r1,0(r0)")
prefer:

(uart-putc ch)
where uart-putc is either:

a compiler-known primitive, or

a Lisp wrapper around a native routine.

That buys you:

one place to fix hardware changes,

emulator instrumentation,

easier tracing,

cleaner future self-hosting.

4. Make (asm ...) verbatim if you keep it
On your indentation question specifically:

If (asm ...) remains a raw textual escape hatch, do not impose formatting policy.

Emit strings verbatim.

Reasoning:

labels,

directives,

comments,

macros,

multi-line emitted text,

future data definitions,

all become simpler if the user fully controls the text.

If you want “pretty” generated .s, pretty-print elsewhere.
Do not make the semantic escape hatch care about indentation.

So I would prefer:

(asm "_panic:"
     "    bra _panic")
over a compiler that automatically prefixes spaces.

That also matches the general precedent of source-level foreign/inline escapes: the payload is treated as the programmer’s text, not reformatted for them. CHICKEN’s foreign interface is conceptually much closer to verbatim insertion than to a machine-aware register allocator. 

5. The best “precedent to follow”
If I had to name one composite precedent for your project, it would be:

“Lisp Machine spirit + CHICKEN-style bootstrap + SBCL-style structured low-level core.”
In practical terms that means:

Lisp Machine spirit

OS mostly in Lisp,

machine substrate is small and explicit,

low-level ops exposed as language primitives.

CHICKEN-style bootstrap

early compiler emits another low-level language,

allow an escape hatch while bootstrapping. 

SBCL-style structure

serious backend/runtime code is structured and machine-aware,

not just random inline blobs. 

That is much stronger than trying to model the whole system after inline assembly.

Recommended decision
Here is the decision I would make in your shoes:

Adopt
Approach 1 only as a bootstrap escape hatch.

Approach 2 as the main native interface.

Approach 3 as the longer-term self-hosting goal.

Plus one addition: define a small set of compiler-known primops now, even before full call-native.

That is mostly your uploaded roadmap already, but I would sharpen the priority:

Priority order I would use
Tiny native substrate in .s

Compiler-known primops

Named native calls / trampoline

Cross-compiled raw (asm ...)

Lisp assembler / LAP later

Your uploaded note currently presents raw inline asm as first priority, runtime FFI second, assembler third. I agree with the sequencing for implementation convenience, but for conceptual architecture, I would place primops/native ABI above raw inline asm as the thing to optimize for. 


Final pros/cons summary
If you choose “raw asm first and mainly”
Benefit: fastest start.
Risk: you gradually build an OS whose true language is ad hoc assembler hidden inside Lisp strings.

If you choose “primops + native ABI first”
Benefit: cleaner architecture, safer growth, better self-hosting path.
Risk: a bit more up-front design work.

If you choose “wait for Lisp assembler first”
Benefit: elegant end state.
Risk: delayed OS progress; likely over-engineering too early.

Bottom line
For an embedded COR24 OS primarily written in Macro Lisp, the strongest precedent is:

Use Lisp as the systems language, but keep the machine interface small, explicit, and structured.

So my advice is:

Yes to (asm ...), but only as a bootstrap escape hatch.

Yes to call-native, and make it central.

Yes to a future Lisp assembler.

No to making raw assembly strings the main way the OS talks to hardware.

Prefer compiler-known primops and named native services as your lasting design.

Your uploaded design is already close to the right road; I would mainly tighten the architecture so the eventual system grows around primops and native service boundaries, not around inline asm text. 

 


If you want, I can turn this into a concrete COR24 Macro Lisp OS architecture sketch with:

boot/runtime layers,

primop list,

native ABI rules,

and a staged implementation plan.


My thoughts:  The target ISA is pretty simple so not a lot of assembler is needed, but I would like to support the Cross-compiled strings approach, with its known risks.  It might be for temporary code that will be turned into Lisp primitives later.  e.g. uses: (asm "_panic:" " bra _panic" ";halt for xxx") as a panic with documentation perhaps; or (asm "nop" "nop" "nop") for a dynamic debugger that looks for 3-nop's and replaces them with a branch to a debugging or trace routine linked in.    To discourage mis-use this facility is not available within the OS, since the Rust-based C compiler is not running there.  For the eventual in-OS use, the DSL approach is more appealing and safer.  The other approach using native .s functions as lisp primitives will be used too (already is) 

Your requirements now look clearer:

Keep cross-compiled raw-string (asm ...) as a deliberate, host-only escape hatch.

Do not make it available inside the eventual COR24-resident OS/toolchain.

Use native .s routines as Lisp primitives for stable low-level services.

Move toward a structured DSL/LAP-like assembler on target for self-hosted development.

Treat raw-string asm as temporary, risky, and intentionally inconvenient.

I think that is a good split, and it resolves most of the architectural tension.

Why this is a sound direction
You are not choosing between “safe” and “unsafe.”
You are choosing where to place unsafe power.

Your current proposal places it in the right place:

host cross-compiler phase: permissive, sharp tools allowed

in-OS self-hosted phase: structured, constrained, safer interfaces

shared long-term substrate: named native routines / primitives

That gives you:

fast experimentation now,

less long-term language pollution,

a clear path to self-hosting,

lower risk of turning the OS into disguised assembler.

What I think you should adopt
1. Keep raw-string (asm ...), but define it as a bootstrap-only facility
That matches your stated intent well.

Benefits
Lets you quickly prototype:

panic loops,

trace hooks,

debugger patch points,

device bring-up,

ISR experiments,

one-off performance fixes.

Avoids over-designing a DSL before you know what patterns matter.

Keeps the low-level escape available during the stage where it has the highest payoff.

Risks
Encourages “just one more asm block” behavior.

Makes some Lisp source harder to reason about.

Can hide ABI violations until runtime.

Why the boundary matters
Your strongest safety mechanism is not syntax.
It is phase separation:

raw asm exists only in the host cross-compiled world, not in the resident OS world.

That is a very effective form of discipline.

2. Make native .s functions the durable low-level interface
This is the right long-term anchor.

You already said this approach is already in use. That is a strong sign that it is practical for your environment.

This should be where you put:

MMIO helpers,

interrupt control,

scheduler/context-switch support,

SPI/I2C/UART kernels,

memory block routines,

trap/debug entry points,

any instruction sequences with register/stack delicacy.

Why this works well
A named native routine gives you:

a stable ABI,

a single place to document calling convention,

a clear audit surface,

easier replacement later,

natural wrapping as Lisp primitives.

That is much better than scattering equivalent behavior across many inline asm snippets.

3. Make the eventual on-target DSL the preferred low-level authoring model
I agree with you that the DSL/LAP-style direction is more appealing for in-OS use.

That is where you get the best balance of:

expressiveness,

structure,

macro power,

label handling,

safer composition,

future analysis/transformation.

You do not need to build the whole thing now, but you should design raw (asm ...) so it does not box you out of that future.

Your example uses make sense
Panic loop
(asm "_panic:" " bra _panic" ";halt for xxx")
Architecturally, this is a good example of the kind of thing raw asm is excellent for:

tiny,

obvious,

machine-specific,

hard to justify as a full primitive at first,

likely to move later into a named runtime stub or primitive.

I would slightly tighten the intended style:

(asm "_panic:"
     "    bra _panic"
     "; halt for xxx")
Mainly for readability.

Debugger patch points with 3 NOPs
(asm "nop" "nop" "nop")
This is also a very sensible use case.

It is:

intentionally machine-aware,

intentionally patchable,

deliberately visible in emitted .s,

a good example of something that is awkward to express as a general Lisp primitive.

This sort of patch-point concept is exactly the kind of thing that often starts as raw emitted code and later becomes a structured form like:

(debug-patch-point 'trace-x)
which the compiler or DSL eventually lowers to three NOPs or some other reserved sequence.

So this is a strong example of why keeping the bootstrap escape hatch is worthwhile.

Suggested policy for raw (asm ...)
To discourage misuse without overcomplicating the implementation, I would adopt explicit rules.

Allowed uses
one-off bootstrap work,

temporary device bring-up,

debug/trace patch points,

panic/trap loops,

tiny leaf instruction sequences,

code that will likely become a primitive or native stub later.

Discouraged uses
large routines,

control-flow-heavy logic,

stack-frame construction,

complex loops,

general driver logic,

anything that should really be a named .s routine.

Strongly discouraged uses
direct mutation of sp/fp except in carefully isolated substrate code,

ad hoc calling convention variations,

embedding whole subsystems as strings.

This gives your future self and any coding agent a policy to follow.

I would make three concrete design choices now
Choice A: emit raw asm strings verbatim
Given your intended use cases, I would not auto-indent.

Why:

labels,

directives,

comments,

patch markers,

mixed formatting,

future pseudo-ops

all work better if the user controls the text.

So the cross-compiler should output the string content as-is, line by line, with only a terminating newline if needed.

That keeps the facility honest:

this is raw textual assembly insertion.

Not “formatted instruction emission.”

Choice B: treat multi-line asm strings as valid
I would allow both:

(asm "_panic:" "    bra _panic")
and

(asm "_panic:\n    bra _panic")
but I would recommend one logical source line per string in style guidance.

Reason:

better diffs,

easier macro generation,

easier diagnostics,

easier future migration to a DSL.

So: support multi-line, style-guide against it for normal use.

Choice C: add an explicit “temporary / bootstrap” naming convention
To reinforce intent, you might even want a naming distinction.

For example:

(asm ...) for raw inline bootstrap assembly

(lap ...) later for structured DSL

native stubs named with a prefix like:

native-uart-putc

native-panic

native-mmio-read24

Or perhaps mark bootstrap uses with comments/macros:

(bootstrap-asm
  "_panic:"
  "    bra _panic"
  "; halt for xxx")
Even if it expands to asm, the name itself communicates policy.

That may help AI agents avoid overusing it.

Pros of your chosen direction
High leverage now
You can keep moving without waiting on:

on-target assembler,

DSL syntax,

codegen metadata,

advanced validation.

Clean separation of eras
You have a practical evolutionary story:

Era 1: raw-string host escape hatch

Era 2: named native services and primitives

Era 3: structured self-hosted assembler DSL

That is much healthier than pretending you can skip straight to era 3.

Better self-hosting outcome
Because raw asm is intentionally absent inside the resident OS, the system is naturally pushed toward:

primitives,

native routines,

DSL assembly,

structured metaprogramming.

That makes the final environment more coherent.

Cons and failure modes to watch for
1. Bootstrap code fossilizes
The main risk is that “temporary” asm remains forever.

Mitigation
Tag such code clearly:

TODO primitive

TODO native-stub

BOOTSTRAP ONLY

And periodically review it.

2. Agents may overuse it
A coding agent will often choose the quickest sharp tool.

Mitigation
Write explicit rules into the project guidance:

prefer Lisp primitive,

then native .s routine,

then DSL/LAP,

raw (asm ...) only when the other choices are not yet available.

3. ABI accidents
Even small inline sequences can accidentally rely on fragile compiler context.

Mitigation
Document the contract for inline asm:

expected live registers,

required preserved registers,

whether result must end in r0,

whether stack/frame access is permitted.

A small but important refinement
I would separate two concepts:

Inline asm expression
Used where the compiled expression expects some result or side effect.

Example:

(define (peek-word addr)
  ... )
Top-level/raw emission
Used for labels, patch areas, directives, standalone fragments.

Example:

(asm "_panic:"
     "    bra _panic")
You can implement both with the same mechanism initially, but conceptually they are different.

Later you may want two forms, such as:

asm = inline expression-oriented raw insertion

asm-top or emit-asm = top-level/module-level emission

That helps avoid ambiguity around whether a construct is meant to “run inline” or “define code/data.”

My overall judgment
Your revised position is strong.

I agree with this model
Raw cross-compiled strings: yes

Host-only: yes

Known risks accepted: yes

Not available in the resident OS: yes

Structured DSL later: yes

Native .s primitives also used: absolutely yes

That gives you the right mix of:

pragmatism,

discipline,

forward compatibility.

In one sentence
Use raw-string asm as a fenced-off bootstrap tool, native .s routines as the real machine boundary, and a DSL as the eventual self-hosted replacement.

That is a good architecture.

If you want, the next useful step is to define a written policy and contract for (asm ...):

exact syntax,

verbatim emission rules,

register/ABI rules,

allowed contexts,

examples of acceptable vs unacceptable use.




