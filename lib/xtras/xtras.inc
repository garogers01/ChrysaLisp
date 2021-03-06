;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; xtras - ChrysaLisp extra forms
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defq xtras_defined t)

; Constants

(defq
  +nl+          (char 0x0a)
  +eof+         (char 0x00)
  +dblq+        (char 0x22)
  +singq+       (char 0x27)
  +kw_ind+      (char 0x3a)
  +upper_chars+ "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  +lower_chars+ "abcdefghijklmnopqrstuvwxyz"
  +bin_prefix+  "0bB"
  +oct_prefix+  "0oO"
  +hex_prefix+  "0xX"
  +oct_chars+   "01234567"
  +bin_chars+   "01"
  +plus_minus+  "-+"
  +dec_point+   "."
  +numeric_chars+ (cat +oct_chars+ "89")
  +hex_chars+ (cat +numeric_chars+ (slice 0 6 +upper_chars+) (slice 0 6 +lower_chars+))
  +alpha_num+ (cat +numeric_chars+ +upper_chars+ +lower_chars+))

; Predicates

(defun-bind numcharset? (_ cset)
  ; (numcharset? val charset) -> t | nil
  (reduced-reduce (#(if (find %1 cset) %0 (reduced nil))) _ t))

(defun-bind intstr? (_)
  ; (intstr? val) -> t | nil
  (cond
    ((not (find +dec_point+ _))
      (numcharset? _ +numeric_chars+))
    (t
      nil)))

(defun-bind decstr? (_)
  ; (decstr? val) -> t | nil
  (if (find +dec_point+ _)
    (progn
      (defq
        psplit (split _ +dec_point+))
      (and
        (= (length psplit) 2)
        (numcharset? (first psplit) +numeric_chars+)
        (numcharset? (second psplit) +numeric_chars+)))
    nil))

(defun-bind _!base10str? (_ prefix charset)
  ; (_!base10str? val prefixchars charset) -> t | nil
  (cond
    ((and
      (eql (first _) (first prefix))
      (or
        (eql (second _) (second prefix))
        (eql (second _) (last prefix))))
      (numcharset? (slice 2 -1 _) charset))
    (t
      nil)))

(defun-bind hexstr? (_)
  (_!base10str? _ +hex_prefix+ +hex_chars+))

(defun-bind octstr? (_)
  (_!base10str? _ +oct_prefix+ +oct_chars+))

(defun-bind binstr? (_)
  (_!base10str? _ +bin_prefix+ +bin_chars+))

(defun-bind numstr-type (_)
  ; (numstr-type el) -> numeric string tuple
  ; Returns the numeric string type tuple, where:
  ; Elem 0 = 1 for positive, -1 for negative
  ; Elem 1 = :hexidecimal | :octal | :binary | :integer | :decimal | :nan
  ; Elem 2 = unprefixed input base portion of string
  ; Elem 3 = decimal portion of string iff Elem 1 is :decimal | nil
  ; Elem 4 = base 16 | 10 | 8 | 2 | nil
  (when (nempty? _)
    (defq res (list
      (if (eql (first _) "-")
        (progn (setq _ (rest _)) -1)
        (if (eql (first _) "+")
          (progn (setq _ (rest _)) 1)
          1))))
      (cond
        ((hexstr? _ )
          (push res :hexidecimal (slice 2 -1 _) nil 16))
        ((octstr? _ )
          (push res :octal (slice 2 -1 _) nil 8))
        ((binstr? _ )
          (push res :binary (slice 2 -1 _) nil 2))
        ((intstr? _)
          (push res :integer _ nil 10))
        ((decstr? _)
          (defq sp (split _ +dec_point+))
          (push res :decimal (first sp) (second sp) 10))
        (t (push res :nan _ nil nil)))))

(defun-bind numstr? (_)
  ; (numstr? string) -> t | nil
  ; Tests whether string is numeric form
  (not (eql (second (numstr-type _)) :nan)))

(defun-bind to-num (s)
  ; (to-num string) -> number
  (bind '(mu ty wn dn ba)  (numstr-type s))
  (when (not (eql ty :nan))
    (case ty
      ((:integer)
        (* (str-to-num wn) mu))
      ((:decimal)
        (* (i2f mu) (+ (i2f (str-to-num wn))
          (/ (i2f (str-to-num dn)) (i2f (pow ba (length dn)))))))
      ((:hexidecimal :octal :binary)
        (defq num 0)
        (each (# (setq num (+ (* num ba) (char-to-num %0)))) wn)
        (* mu num)))))

(defun-bind zero? (_)
  ; (zero? num) -> bool
  ; Returns true if argument is equal to zero (0)
  (= _ 0))

(defun-bind one? (_)
  ; (one? num) -> bool
  ; Returns true if argument is equal to one (1)
  (= _ 1))

(defun-bind neql? (a b)
  ; (neql? value value) -> bool
  ; Returns true if values are not eql
  (not (eql a b)))

(defun-bind dict? (_)
  ; (dict? form) -> bool
  ; Returns true if argument is a dictionary type (env)
  (env? _))

(defun-bind props? (_)
  ; (props? el) -> bool
  ; Returns true if argument is a properties type
  (when (and
    (lst? _)
      (eql (getp _ :super_clz) :clz_properties))
    t))

(defun-bind def? (_ &optional e)
  ; (def? sym [env]) -> bool
  ; Returns t if sym is defined in environment
  ; Defaults to current environment if not provided
  (setd e (env))
  (not (eql (get _ e) nil)))

(defun-bind ndef? (_ &optional e)
  ; (ndef? sym [env]) -> bool
  ; Returns t if sym is not defined in environment
  (not (def? _ e)))

(defun-bind kw? (_)
  ; (kw? el) -> bool
  ; Returns true if argument is a keyword
  (and (sym? _) (starts-with ":" _)))

(defun-bind truthy? (_)
  (cond
    ((lst? _) (nempty? _))
    (t (not (nil? _)))))

; Convenience

(defun-bind str-is-ints? (s)
  ; (str-is-ints? s) -> :true | :false
  (defq instr
    (if (find (first s) +plus_minus+)
      (rest s)
      s))
  (reduced-reduce
      (lambda (acc el)
        (if  (find el +numeric_chars+)
          acc
          (reduced :false))
        ) instr :true))

(defun-bind ident (_)
  ; (ident el) -> el
  _)

(defun-bind numb-from-string (inumstr)
  (defq accum 0 base 10 deci 0 mult 1)

  )
(defun-bind efind (p k)
  ; (efind list key) -> index | nil
  ; Returns the index of key in list or properties
  ; Used when keys are number and/or string
  (defq fx nil)
  (each! 0 -1
    (lambda (s)
      (when (eql s k)
        (setq fx _))) (list p))
  fx)

(defun-bind kw (_)
  ; (kw el) -> keyword
  ; Converts a string or symbol to a keyword
  ; example:
  ; (kw "foo") -> :foo
  ; (kw ":foo") -> :foo
  (if (and (not (kw? _)) (or (str? _) (sym? _)))
    (if (eql (first _) +kw_ind+)
      (sym _)
      (sym (str +kw_ind+ _)))
    (throw "Expected symbol or string, found " _)))

(defun-bind strip-rest (in-args)
  ; (strip-rest in-args)
  ; Returns inner list if nested from passing
  ; arguments through multiple &rest constructs
  (if (and
        (nempty? in-args)
        (lst? (first in-args)))
      (first in-args)
      in-args))

; Sequence forms

(defmacro-bind interleave (seq1 seq2)
  ; (interleave seq1 seq2) -> list
  ; Returns a list of the first item in each seq, then the second etc.
  ; example:
  ; (interleave '(:a :b :c) '(0 1 2)) -> (:a 0 :b 1 :c 2)
  `(zip ,seq1 ,seq2))

(defun-bind chunk (cnt seq)
  ; (chunk count sequence) -> sequence of sequences
  ; Chunks up a sequence into list of sequence by count
  ; example:
  ; (chunk 2 '(0 1 2 3')) -> ((0 1) (2 3))
  (cond
    ((< cnt (length seq))
      (defq
        base (reverse (copy seq))
        res (list))
      (while (not (empty? base))
        (defq nl (list))
        (times cnt (push nl (pop base)))
        (push res nl))
      res)
    (t (list seq))))

; (defun-bind unions (&rest lists)
;   ; (unions [list]...) -> list
;   (defq nl (list))
;   (each (curry merge nl) lists))

; (defun-bind unions-with (base &rest lists)
;   ; (unions-with list [list]...) -> list'
;   (each (curry merge base) lists))

; Properties (a.k.a. list of pairs [k v]) forms

(defmacro-bind properties (&rest pairs)
  ; (properties [k v]...) -> list
  ; Creates a properties list object
  (push pairs :super_clz :clz_properties)
  `(list ~pairs))

(defun-bind entries (_)
  ; (entries properties) -> entry list
  ; Returns a list of entries where each entry: (key value)
  (when (props? _)
    (defq c (chunk 2 _))
    (reduce (lambda (acc e)
      (if (or (eql (first e) :clz) (eql (first e) :super_clz))
        acc
        (push acc e)))
      c (list))))

(defun-bind keys (prop)
  ; (keys properties) -> key list
  ; Returns a sequence of keys found in properties list
  (map (#(elem 0 %0)) (entries prop)))

(defun-bind values (prop)
  ; (values properties) -> value list
  ; Returns a sequence of values found in properties list
  (map (#(elem 1 %0)) (entries prop)))

(defun-bind getp (p k &optional if_nil)
  ; (getp properties key [if_nil]) -> val | if_nil | nil
  (defq fx nil)
  (cond
    ((or (kw? k) (sym? k))
      (setq fx (find k p)))
    ((or (num? k) (str? k))
      (setq fx (efind p k)))
    (t nil))
  (if fx (elem (inc fx) p) (opt nil if_nil)))

(defun-bind getp-in (p &rest kws)
  ; (getp-in (properties kw...)) -> value | nil
  ; Returns the value in a nested properties structure
  ; where kws is a sequence (list) of keys. Returns nil
  ; if a key is not present
  (defq hit nil)
  (cond
    ((not (props? p))
      (throw "Not a properties structure " p))
    ((empty? kws)
      (throw "No keys path provided " kws))
    (t
      (defq
        keep_going t
        last_nest  p
        pick (reverse kws)
        fkw  (last pick))
      (while (and keep_going (not (empty? pick)))
        (defq v (getp last_nest fkw))
        (pop pick)
        (cond
          ((not v)
            (setq keep_going nil))
          ((empty? pick)
            (setq
              keep_going nil
              hit v))
          (t
            (setq
              fkw       (last pick)
              last_nest v))))))
    hit)

(defun-bind setp! (p k v &optional add_if_nil)
  ; (setp properties k value [add_if_nil]) -> properties
  (defq ki (if (kw? k) (find k p) (efind p k)))
  (if (not ki)
    (if add_if_nil
      (push p k v)
      (throw (str "Key " k " does not exist") p))
    (elem-set (inc ki) p v))
  p)

(defun-bind setsp! (p &rest pairs)
  ; (setsp properties [k v] ...) -> properties
  (each (#(setp! p (first %0) (second %0) t)) (chunk 2 pairs))
  p)

(defun-bind setsp-in! (p val &rest kws)
  ; (setsp-in! properties value kw...) -> properties
  ; Sets the value in a nested properties structure
  ; where kws is a sequence of keys.
  (cond
    ((not (props? p))
      (throw "Not a properties structure " p))
    ((empty? kws)
      (throw "No keys path provided " kws))
    (t
      (defq
        keep_going t
        last_nest  p
        pick (reverse kws)
        fkw  (last pick)
        trg  (first pick))
      (while (and keep_going (not (empty? pick)))
        (defq v (getp last_nest fkw))
        (pop pick)
        (cond
          ((not v)
            (setq keep_going nil))
          ((empty? pick)
            (if (eql trg fkw)
              (setp! last_nest trg val))
            (setq keep_going nil))
          (t (setq
            fkw       (last pick)
            last_nest v))))))
  p)

(defun-bind update-in! (p ufnc &rest kws)
  ; (update-in! properties ufnc kw...)) -> value | nil
  ; Updates the value found in the key-leaf of kws with the
  ; result of calling ufnc with the current value
  ; Returns nil if a key is not present
  (defq hit nil)
  (cond
    ((not (props? p))
      (throw "Not a properties structure " p))
    ((empty? kws)
      (throw "No keys path provided " kws))
    (t
      (defq
        keep_going t
        last_nest  p
        pick (reverse kws)
        fkw  (last pick))
      (while (and keep_going (not (empty? pick)))
        (defq v (getp last_nest fkw))
        (pop pick)
        (cond
          ; If key not found exit
          ((not v)
            (setq keep_going nil))
          ; Otherwise, if we are at leaf we
          ; have our target entry
          ((empty? pick)
            (setq
              keep_going nil
              hit (ufnc (elem (inc (efind last_nest fkw)) last_nest)))
            (setp! last_nest fkw hit))
          (t
            (setq
              fkw       (last pick)
              last_nest v))))))
    hit)

(defun-bind pmerge (&rest props)
  ; (pmerge properties...) -> properties
  ; Returns a new properties structure that consists of
  ; the rest of the properties structures merged into
  ; it
  (reduce
    (#(each (lambda (e)
      (setp! %0 (first e) (second e) t)) (chunk 2 %1)))
    props (properties)))

(defun-bind move-entry-forward! (p sp ep)
  ; (move-entry-forward! properties start-key-index end-key-index)
  ; Moves the properties k/v pair forward from key index sp to index ep
  (while (/= sp ep)
    (defq tsp (+ sp 2))
    (swap p sp tsp)
    (swap p (inc sp) (+ sp 3))
    (setq sp tsp))
  p)

(defun-bind move-entry-backward! (p sp ep)
  ; (move-entry-backward! properties start-key-index end-key-index)
  ; Moves the properties k/v pair backward from key index sp to index ep
  (while (/= sp ep)
    (defq tsp (- sp 2))
    (swap p sp tsp)
    (swap p (inc sp) (dec sp))
    (setq sp tsp))
  p)

(defun-bind pdrop! (p k)
  ; (pdrop properties key) -> properties
  ; Drops a properties key/pair
  (when (props? p)
    (defq
      sp (efind p k)
      ep (- (length p) 2))
    (when sp
      (move-entry-forward! p sp ep)
      (times 2 (pop p)))))

; Privatize functions

(defmacro private (nm)
  ; (private nm) -> env
  ; Sets up an isolated environment for
  ; use in the privatization of functions
  `(macroexpand (defq ,nm (env -1))))

(defmacro defunp (e n a &rest _)
  ; (defunp env name args form) -> lambda
  ; Declares a function in the private environment
  `(def ,e ,n (lambda ,a ~_)))

(defmacro defunp-bind (e n a &rest _)
  ; (defun-bind name ([arg ...]) body)
  ; Declars a bound function in the private environment
  `(def ,e ,n (lambda ,a ~(prebind (macroexpand _)))))

(defmacro callp (e name &rest _)
  ; (callp env name &rest args) -> form invoked with args
  ; Calls a function in a private environment
  `((get ,name ,e) ~_))

; Walkers

(defun walk-tree (tree fnc)
  (defq stack (if (> (length tree) 0) (list tree 0) '()))
  (while (defq i (pop stack) l (pop stack))
    (defq e (elem i l))
    (if (< (setq i (inc i)) (length l))
        (push stack l i))
    (if (lst? e)
      (if (> (length e) 0) (push stack e 0))
      (fnc e))))

(defun-bind walk-tree-recur (inner-fn outer-fn tree)
  ; (walk-tree-recur inner-fn outer-fn tree) -> tree
  ; Recusively walk tree and handles dictionaries
  (cond
    ((lst? tree)
      (outer-fn
        (apply list
          (map (lambda (el)
            (if (dict? el)
              (walk-tree-recur inner-fn outer-fn el)
              (inner-fn el))) tree))))
    ((dict? tree)
      (map (curry walk-tree-recur inner-fn outer-fn) (dict-entries tree)))
    (t (outer-fn tree))))

(defun-bind pre-walk-recur (fn tree)
  ; (pre-walk-recur fn tree)
  (walk-tree-recur (curry pre-walk-recur fn) ident (fn tree)))

(defun-bind post-walk-recur (fn tree)
  ; (post-walk-recur fn tree)
  (walk-tree-recur (curry post-walk-recur fn) fn tree))

; Take and drop forms

(defun-bind take (cnt seq)
  ; (take count sequence) -> sequence
  ; Returns the first `cnt` of elements from seq
  ; providing a negative cnt returns the original sequence
  ; example:
  ; (take 2 '(0 1 2 3)) -> (0 1)
  (defq len (length seq))
  (cond
    ((neg? cnt) seq)
    (t (slice 0 (cond ((> cnt len) len) (t cnt)) seq))))


(defun-bind take-last (cnt seq)
  ; (take-last count sequence) -> sequence
  ; Returns the last 'cnt' of elements from seq
  ; providing a negative cnt returns the original sequence
  ; example:
  ; (take-last 2 '(0 1 2 3)) ->  (2 3)
  (reverse (take cnt (reverse seq))))

(defun-bind take-while (pred seq)
  ; (take-while predicate sequence) -> sequence
  ; Returns successive element of seq while pred returns true
  ; example:
  ; (take-while neg? '(-1 -2 0 -1 -2)) -> (-1 -2)
  (reduced-reduce
    (lambda (acc el)
      (cond
        ((pred el) (push acc el))
        (t (reduced acc))))
    seq (list)))

(defun-bind take-unless (pred seq)
  ; (take-unless predicate sequence) -> sequence
  ; Returns successive element of seq unless pred returns true
  ; example:
  ; (take-unless (# (< %0 0)) '(-1 -2 0 -1 -2)) -> (0)
  (reduced-reduce
    (lambda (acc el)
      (cond
        ((pred el) acc)
        (t (push acc el))))
    seq (list)))

(defun-bind drop (cnt seq)
  ; (drop count sequence) -> sequence
  ; Returns a sequence with the first `cnt` of elements removed
  ; providing a negative cnt returns the original sequence
  ; example:
  ; (defq myL2 (list -2 -1 0 -1 -2 3))
  ; (drop 2 myL2)       ; -> (0 -1 -2 3)
  (defq len (length seq))
  (cond
    ((neg? cnt) seq)
    (t (slice (cond ((> cnt len) -1) (t cnt)) -1 seq))))

(defun-bind drop-last (cnt seq)
  ; (drop-last count sequence) -> sequence
  ; providing a negative cnt returns the original sequence
  ; example:
  ; (defq myL2 (list -2 -1 0 -1 -2 3))
  ; (drop-last 2 myL2)  ; -> (-2 -1 0 -1)
  (reverse (drop cnt (reverse seq))))

(defun-bind drop-while (pred seq)
  ; (drop-while predicate seq) -> sequence
  ; Drops successive element of seq while pred returns true
  ; example:
  ; (defq myL2 (list -2 -1 0 -1 -2 3))
  ; (drop-while (# (< %0 0)) myL2)  ; -> (0 -1 -2 3)
  (slice (length (take-while pred seq)) -1 seq))

(defun-bind drop-unless (pred seq)
  ; (drop-unless predicate sequence) -> sequence
  ; Drops elements from sequence unless predicate returns true
  ; example:
  ; (defq myL2 (list -2 -1 0 -1 -2 3))
  ; (drop-unless (# (< %0 0)) myL2) ; -> (-2 -1 -1 -2)
  (reduced-reduce
    (lambda (acc el)
      (cond
        ((pred el) (push acc el))
        (t acc)))
    seq (list)))

; Data Object AST (Node Tree)

(defun Node (ttype &optional value)
  (properties
      :type         ttype
      :value        value
      :children     (list)))

(defun DocStartNode ()
  (Node :docstart "---"))

(defun DocEndNode ()
  (Node :docend "..."))

(defun MapNode ()
  (Node :map))

(defun MapEntryNode ()
  (Node :map_entry))

(defun KeyNode ()
  (Node :key))

(defun ValueNode ()
  (Node :value))

(defun SequenceNode ()
  (Node :seq))

(defun ScalarNode (stype value)
  (setp! (Node :scalar value) :stype stype t))

(defun add-child-node! (p c)
  (when (not (lst? (defq chs (getp p :children))))
    (throw "Children of node is not a list" p))
  (push chs c)
  c)

(defun me-key? (_)
  (eql (getp _ :clz) :me_key))

(defun me-value? (_)
  (eql (getp _ :clz) :me_value))

(defun prop-me? (_)
  (eql (getp _ :clz) :prop_me))

(defun make-me-key (_)
  (list :clz :me_key :value _))

(defun make-me-value (_)
  (list :clz :me_value :value _))

(defun split-entries (p)
  (reduce
    (lambda (acc el)
      (push acc
        (list :clz :prop_me :entries
              (push
                (push (list) (make-me-key (first el)))
                (make-me-value (second el))))))
    (entries p) (list)))

; AST Node Context Stack

(defun Context ()
  ; (Context) -> properties
  ; Constructs a common node stack context
  (properties
    :current nil
    :path    (list)))

(defun set-context! (cntxt n)
  ; (set-context! node) -> node
  ; Makes the current context 'node'
  ; after adding 'node' to current's children
  (defq crn (getp cntxt :current))
  ; Stack node in path for un-setting
  (push (getp cntxt :path) n)
  ; If first
  (when crn
    (add-child-node! crn n))
  (setp! cntxt :current n)
  n)

(defun unset-context! (cntxt)
  ; (unset-context!) -> node | nil
  ; Set's context to most recent in path
  (defq
    npath (getp cntxt :path)  ; Setup
    lnode (pop npath))          ; Pop path stack
  (when (truthy? npath)
    (setp! cntxt :current (last npath)))
  nil)

(defun add-to-context! (cntxt n)
  ; (add-to-context! node) -> node | nil
  (when (truthy? (defq crn (getp cntxt :current)))
    (add-child-node! crn n)))

(defun obj-to-node (cntxt odata)
  ; (obj-to-node Context object)
  ; Builds an common Node AST from object
  (cond
    ((lst? odata)
        (cond
          ; Map
          ((props? odata)
           (set-context! cntxt (MapNode))
           (each (#(obj-to-node cntxt %0)) (split-entries odata))
           (unset-context! cntxt))
          ; Map Entry
          ((prop-me? odata)
           (set-context! cntxt (MapEntryNode))
           (each (#(obj-to-node cntxt %0)) (getp odata :entries))
           (unset-context! cntxt))
          ; Key
          ((me-key? odata)
           (set-context! cntxt (KeyNode))
           (obj-to-node cntxt (getp odata :value))
           (unset-context! cntxt))
          ; Value
          ((me-value? odata)
           (set-context! cntxt (ValueNode))
           (obj-to-node cntxt (getp odata :value))
           (unset-context! cntxt))
          ; Sequence
          (t
            (set-context! cntxt (SequenceNode))
            (each (#(obj-to-node cntxt %0)) odata)
            (unset-context! cntxt))))
    ((or (eql odata t) (eql odata nil))
     (add-to-context! cntxt (ScalarNode :boolean odata)))
    ((kw? odata)
     (add-to-context! cntxt (ScalarNode :keyword odata)))
    ((sym? odata)
     (add-to-context! cntxt (ScalarNode :symbol odata)))
    ((str? odata)
     (add-to-context! cntxt (ScalarNode :string (cat +dblq+ odata +dblq+))))
    ((num? odata)
     (add-to-context! cntxt (ScalarNode :number odata)))
    (t
     (throw "Unknown type found in obj-to-node" odata))))

; Pretty Printers
(defq ppopts
  (properties
    :map-delims   "{}"
    :list-delims  "()"
    :indent       -1
    :indent_space  2))

(defq pp_boolean
      (properties
        t   "true"
        nil "false"))

(defun indent+ (cntxt)
  (defq ci (getp cntxt :indent))
  (setp! cntxt :indent (inc ci)))

(defun indent- (cntxt)
  (defq ci (getp cntxt :indent))
  (setp! cntxt :indent (dec ci)))

(defun padded= (cntxt)
  (pad "" (* (getp cntxt :indent_space) (getp cntxt :indent))))

(defun padded- (cntxt)
  (pad "" (* (getp cntxt :indent_space) (dec (getp cntxt :indent)))))

(defun pp-write-pad (sstrm cntxt)
  (write sstrm (padded= cntxt)))

(defun pp-niw (sstrm cntxt el)
  (write sstrm el))

(defun pp-iw (sstrm cntxt el)
  (write (pp-write-pad sstrm cntxt) el))

(defun pp-iwcr (sstrm cntxt el)
  (write (pp-iw sstrm cntxt el) +nl+))

(defun pp-map-container (sstrm cntxt el)
  (write (pp-write-pad (write sstrm +nl+) cntxt) (str el +nl+)))

(defun node-to-pprint (sstrm in-args ast)
  (defq recur (curry node-to-pprint sstrm in-args))
  (case (getp ast :type)
    ((:map)
      (indent+ in-args)
      (pp-map-container sstrm in-args (first (getp in-args :map-delims)))
      (indent+ in-args)
      (each recur (getp ast :children))
      (indent- in-args)
      (if (= (getp in-args :indent) 1)
        (pp-iwcr sstrm in-args (last (getp in-args :map-delims)))
        (pp-iw sstrm in-args (last (getp in-args :map-delims))))
      (indent- in-args))
    ((:seq)
     (indent+ in-args)
     (pp-niw sstrm in-args (first (getp in-args :list-delims)))
     (indent+ in-args)
     (each recur (getp ast :children))
     (indent- in-args)
     (pp-niw sstrm in-args (last (getp in-args :list-delims)))
     (indent- in-args))
    ((:map_entry)
     (each recur (getp ast :children)))
    ((:key)
      (pp-iw sstrm in-args "")
      (each recur (getp ast :children)))
    ((:value)
      (each recur (getp ast :children))
      (pp-iwcr sstrm in-args ""))
    ((:scalar)
     (if (eql (getp ast :stype) :boolean)
        (pp-niw sstrm in-args (str (getp pp_boolean (getp ast :value)) " "))
        (pp-niw sstrm in-args (str (getp ast :value) " "))))
    (t (throw "Unknown " ast))))

(defun-bind pprint-to-strm (sstrm obj out-args)
  (defq context
    (cond
      ((or (empty? out-args) (odd? out-args))
        (pmerge (Context) ppopts))
      (t
        (pmerge (Context) ppopts out-args))))
  (obj-to-node context obj)
  (node-to-pprint sstrm context (getp context :current))
  sstrm)

(defun-bind pprint (obj &rest out-args)
  (print (str (pprint-to-strm (string-stream (cat "")) obj out-args)))
  nil)
