# Libs
import argparse

class ArgumentParser:
    """
    Argument parser singleton
    """
    
    _instance = None
    parser: argparse.ArgumentParser
    parsed_args: argparse.Namespace

    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super(ArgumentParser, cls).__new__(cls)
        return cls._instance

    def __init__(self, debug: bool = False):
        if not hasattr(self, "initialized"):
            self.debug = debug
            self.parser = argparse.ArgumentParser(description="PR Linter")
            self.parser.add_argument("--github", action="store_true", help="Enable GitHub mode", default=True)
            self.parser.add_argument("--pr", type=int, help="PR number")
            self.parser.add_argument("--debug", action="store_true", help="Enable debug mode", default=self.debug)
            self.parser.add_argument("--file", type=str, help="Path to file in root")
            self.parser.parse_args()
            
            self.get_args()
            
            if not self.parsed_args.debug and self.parsed_args.file:
                print("Cannot specify file without debug mode")
                raise SystemExit(1)
            
            if not self.parsed_args.github and self.parsed_args.pr:
                print("Cannot specify PR without GitHub mode")
                raise SystemExit(1)
            
            self.initialized = True

    def get_args(self):
        if not hasattr(self, "parsed_args"):
            self.parsed_args = self.parser.parse_args()
        return self.parsed_args