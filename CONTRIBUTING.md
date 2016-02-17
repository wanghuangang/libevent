# Contributing to the libevent

## Coding style

First and most generic rule: just look around.

- You can check the whole file with `uncrustify`:
```shell
export UNCRUSTIFY_CONFIG=/path/to/file.c
uncrustify /path/to/file.c
diff -u /path/to/file.c /path/to/file.c.uncrustify
```

Or just you hunk/patch:
```bash
export UNCRUSTIFY_CONFIG=/path/to/file.c
function uncrustify_frag()
{ command uncrustify -l C --frag "$@"; }
function indent_off() { echo '/* *INDENT-OFF* */'; }
function indent_on() { echo '/* *INDENT-ON* */'; }
function hunk()
{
    local ref=$1 f=$2
    shift 2
    git cat-file -p $ref:$f
}
function indent_hunk()
{
    local start=$1 end=$2
    shift 2

    # Will be beatier with tee(1), but doh bash async substitution
    { indent_off; hunk "$@" | head -n$((start - 1)); }
    { indent_on;  hunk "$@" | head -n$((end - 1)) | tail -n+$start; }
    { indent_off; hunk "$@" | tail -n+$((end + 1)); }
}
function strip()
{
    local start=$1 end=$2
    shift 2

    # seek indent_{on,off}()
    let start+=2
    head -n$end | tail -n+$start
}
function uncrustify_git()
{
    local ref=$1 r f start end length
    shift

    local files=( $(git diff --name-only $ref^..$ref | egrep "\.(c|h)$") )
    for f in "${files[@]}"; do
        local ranges=( $(git diff -W $ref^..$ref -- $f | egrep -o '^@@ -[0-9]+(,[0-9]+|) \+[0-9]+(,[0-9]+|) @@' | cut -d' ' -f3) )
        for r in "${ranges[@]}"; do
            [[ ! "$r" =~ ^\+([0-9]+)(,([0-9]+)|)$ ]] && continue
            start=${BASH_REMATCH[1]}
            [ -n "${BASH_REMATCH[3]}" ] && \
                length=${BASH_REMATCH[3]} || \
                length=1
            end=$((start + length))
            echo "Range: $start:$end ($length)" >&2

            diff -u \
                <(indent_hunk $start $end $ref $f | strip $start $end) \
                <(indent_hunk $start $end $ref $f | uncrustify_frag | strip $start $end) \
            | sed \
                -e "s#^--- /dev/fd.*\$#--- a/$f#" \
                -e "s#^+++ /dev/fd.*\$#+++ b/$f#"
        done
    done
}
```

- Or you can check patches with `clang-format-diff`:
```
ln -s /path/to/clang-format-config .clang-format
git show origin/master | sed -e 's#^--- a/#--- #' -e 's#^+++ b/#+++ #' | clang-format-diff-3.8
```

- [uncrustify config](https://strcpy.net/mark/libevent-uncrustify.cfg)
- [clang-format config](https://gist.github.com/azat/ea7aa1d55359b1b4917e/raw)

## Testing
- Write new unit test in `test/regress_{MORE_SUITABLE_FOR_YOU}.c`
- ` make verify `
