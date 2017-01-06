#!/usr/bin/env python3

import os, sys, argparse, logging
from termcolor import colored
import vagrant
import subprocess, threading

class Box(vagrant.Vagrant):
    def __init__(self, name, **kwargs):
        self.name = name
        self.timeout = kwargs.pop('timeout', 0)
        self.verbose = kwargs.pop('verbose', False)
        self.no_pkg = kwargs.pop('no_pkg', False)
        self._env = os.environ.copy()
        self.last = False

        vagrant.Vagrant.__init__(
            self,
            err_cm=vagrant.make_file_cm(self._log("stderr")),
            out_cm=vagrant.make_file_cm(self._log("stdout")),
        )

    def run(self):
        tests = {}

        prepared = True
        if not self.no_pkg:
            prepared = self._run("prepare", "NO_PKG")

        tests["{}_prepare".format(self.name)] = prepared

        if prepared:
            tests["{}_cmake".format(self.name)] = self._run("cmake", "NO_CMAKE")
            tests["{}_autotools".format(self.name)] = self._run("autotools", "NO_AUTOTOOLS")
        else:
            tests["{}_cmake".format(self.name)] = False
            tests["{}_autotools".format(self.name)] = False

        return tests
    def _run(self, which, env):
        self._unlink(self._log("stderr"))
        self._unlink(self._log("stdout"))

        self.info(which)
        self._reset_env(env)

        def target():
            self.debug("starting")
            self.last = self.up()
            self.debug("finished: result={}".format(self.last))

        thread = threading.Thread(target=target)
        thread.start()
        thread.join(self.timeout)
        self.debug("joining")

        if thread.is_alive():
            self.warning("halting")
            self.halt()
            thread.join()

        if self.last:
            return True
        else:
            self.print_logs()
            return False

    def up(self):
        try:
            vagrant.Vagrant.up(
                self,
                vm_name=self.name,
                provision=True,
            )
        except subprocess.CalledProcessError:
            return False
        return True
    def halt(self):
        try:
            vagrant.Vagrant.halt(self)
        except subprocess.CalledProcessError:
            pass

    def print_logs(self):
        if not self.verbose:
            return
        self.debug("stdout")
        print(self.stdout())
        self.debug("stderr")
        print(self.stderr())

    def stdout(self):
        return self._read(self._log("stdout"))
    def stderr(self):
        return self._read(self._log("stderr"))

    def info(self, message):
        logging.info("box[name={}] {}".format(self.name, message))
    def debug(self, message):
        logging.debug("box[name={}] {}".format(self.name, message))

    def _reset_env(self, key):
        self.env = self._env
        self.env['NO_PKG'] = "true"
        self.env['NO_CMAKE'] = "true"
        self.env['NO_AUTOTOOLS'] = "true"
        self.env[key] = "false"
    def _log(self, std):
        return ".vagrant/{}_{}.log".format(std, self.name)

    @staticmethod
    def _read(path):
        with open(path) as fp:
            return fp.read()

    @staticmethod
    def _unlink(path):
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass

def main():
    p = argparse.ArgumentParser()
    p.add_argument("-v", "--verbose", action="count")
    p.add_argument("-b", "--boxes", nargs="+")
    p.add_argument("-t", "--timeout", type=int, default=60*60*3)
    p.add_argument("--no-pkg", action="store_true")
    args = p.parse_args()

    logging.basicConfig(
        format="%(asctime)s: %(levelname)s: %(module)s: %(message)s",
        level=logging.DEBUG if args.verbose else logging.INFO,
    )

    names = []
    for v in vagrant.Vagrant().status():
        logging.debug("box[name={}]".format(v.name))
        names.append(v.name)

    if args.boxes:
        for b in args.boxes:
            if not b in names:
                raise ValueError(b)
        names = args.boxes

    tests = {}
    boxes_args = {
        "verbose": args.verbose,
        "timeout": args.timeout,
        "no_pkg": args.no_pkg,
    }
    for name in names:
        box = Box(name, **boxes_args)
        tests = { **tests, **box.run() }

    failures = 0
    for test, status in tests.items():
        if not status:
            print(colored("FAILED {}".format(test), "red"))
            ++failures
        else:
            print(colored("PASSED {}".format(test), "green"))

    sys.exit(0 if failures != 0 else 1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(1)
