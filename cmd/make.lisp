;imports
(import 'class/lisp.inc)
(import 'cmd/asm.inc)

(defun-bind make-doc ()
	(defq *abi* (abi) *cpu* (cpu))
	(defun-bind trim-whitespace (_)
		(trim-start (trim-start _ (ascii-char 9)) " "))
	(defun-bind trim-cruft (_)
		(sym (trim-start (trim-end _ ")") "'")))
	(defun-bind trim-parens (_)
		(sym (trim-start (trim-end _ ")") "(")))
	(defun-bind chop (_)
		(when (defq e (find (char 0x22) _))
			(setq _ (slice 0 e _))
			(sym (slice (inc (find (char 0x22) _)) -1 _))))
	(print "Scanning source files...")
	(defq *imports* (list 'make.inc) classes (list) functions (list)
		docs (list) syntax (list) state 'x)
	(within-compile-env (lambda ()
		(include 'sys/func.inc)
		(each include (all-class-files))
		(each-mergeable (lambda (_)
			(each-line (lambda (_)
				(setq _ (trim-end _ (ascii-char 13)))
				(defq l (trim-whitespace _))
				(when (eql state 'y)
					(if (and (ge (length l) 1) (eql (elem 0 l) ";"))
						(push (elem -2 docs) (slice 1 -1 l))
						(setq state 'x)))
				(when (and (eql state 'x) (ge (length l) 10) (eql "(" (elem 0 l)))
					(defq s (split l " ") _ (elem 0 s))
					(cond
						((eql _ "(include")
							(insert *imports* (trim-cruft (elem 1 s))))
						((eql _ "(def-class")
							(push classes (list (trim-cruft (elem 1 s))
								(if (eq 3 (length s)) (trim-cruft (elem 2 s)) 'null))))
						((eql _ "(dec-method")
							(push (elem -2 classes) (list (trim-cruft (elem 1 s)) (trim-cruft (elem 2 s)))))
						((eql _ "(def-method")
							(setq state 'y)
							(push docs (list))
							(push functions (f-path (trim-cruft (elem 1 s)) (trim-cruft (elem 2 s)))))
						((or (eql _ "(def-func") (eql _ "(defcfun"))
							(setq state 'y)
							(push docs (list))
							(push functions (trim-cruft (elem 1 s))))
						((and (or (eql _ "(call") (eql _ "(jump")) (eql (elem 2 s) "'repl_error"))
							(if (setq l (chop l))
								(insert syntax l)))))) _)) *imports*)))
	;create classes docs
	(sort (lambda (x y)
		(cmp (elem 0 x) (elem 0 y))) classes)
	(defq stream (string-stream (cat "")))
	(write-line stream (cat "# Classes" (ascii-char 10)))
	(each (lambda ((class super &rest methods))
		(write-line stream (cat "## " class (ascii-char 10)))
		(write-line stream (cat "Super Class: " super (ascii-char 10)))
		(each (lambda ((method function))
			(write-line stream (cat "### " class "::" method " -> " function (ascii-char 10)))
			(when (and (defq i (find function functions))
					(ne 0 (length (elem i docs))))
				(write-line stream "```lisp")
				(each (lambda (_)
					(write-line stream _)) (elem i docs))
				(write-line stream (cat "```" (ascii-char 10))))) methods)) classes)
	(save (str stream) 'doc/CLASSES.md)
	(print "-> doc/CLASSES.md")

	;create lisp syntax docs
	(each-line (lambda (_)
		(setq _ (trim-end _ (ascii-char 13)))
		(defq l (trim-whitespace _))
		(when (eql state 'y)
			(if (and (ge (length l) 1) (eql (elem 0 l) ";"))
				(insert syntax (sym (slice 1 -1 l)))
				(setq state 'x)))
		(when (and (eql state 'x) (ge (length l) 10) (eql "(" (elem 0 l)))
			(defq s (split l " ") _ (elem 0 s))
			(cond
				((or (eql _ "(defun") (eql _ "(defmacro") (eql _ "(defun-bind") (eql _ "(defmacro-bind"))
					(setq state 'y))))) 'class/lisp/boot.inc)
	(sort cmp syntax)
	(defq stream (string-stream (cat "")))
	(write-line stream (cat "# Syntax" (ascii-char 10)))
	(each (lambda (_)
		(defq s (split _ " ") form (trim-parens (elem 0 s)))
		(when (eql "(" (elem 0 (elem 0 s)))
			(write-line stream (cat "## " form (ascii-char 10)))
			(write-line stream (cat _ (ascii-char 10))))) syntax)
	(save (str stream) 'doc/SYNTAX.md)
	(print "-> doc/SYNTAX.md"))

;initialize pipe details and command args, abort on error
(when (defq slave (create-slave))
	(defq args (map sym (slave-get-args slave)) all (find 'all args)
		boot (find 'boot args) platforms (find 'platforms args) doc (find 'doc args))
	(cond
		((and boot all platforms) (remake-all-platforms))
		((and boot all) (remake-all))
		((and boot platforms) (remake-platforms))
		((and all platforms) (make-all-platforms))
		(all (make-all))
		(platforms (make-platforms))
		(boot (remake))
		(doc (make-doc))
		(t (make))))
