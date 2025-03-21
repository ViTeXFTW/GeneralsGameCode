import re
from enum import Enum, auto

class LintError:
    """
    A class that represents a linting error in a PR.
    """
    
    def __init__(self, line: int, message: str):
        self.line = line
        self.message = message
        
    @staticmethod
    def stringify(errors: list["LintError"]) -> str:
        return "\n".join([str(error) for error in errors])
    
    def __str__(self):
        return f"Line {self.line}: {self.message}"

class InputTypeBody(Enum):
    TEXT = auto()
    BULLETLIST = auto()
    CHECKLIST = auto()
    GH_REFERENCE = auto()
    ANY = auto()

class InputType:
    """
    A class that represents a header and allowed body type for a PR.
    """
    
    header: str
    input_type: InputTypeBody
    
    def __init__(self, header: str, input_type: InputTypeBody, required: bool = True):
        self.header = header
        self.input_type = input_type
        self.required = required
    
    def get_inputType(self) -> InputTypeBody:
        return self.input_type
    
    def validate(self, body: str, line: int = -1) -> LintError | None:
        """
        Validate the body of the header based on the input type.
        """
        
        # Instead of skipping if not required, always validate
        if body.strip() == "":
            return LintError(line, f"Body for header '{self.header}' is empty")
            
        match(self.input_type):
            case InputTypeBody.TEXT:
                return
        
            case InputTypeBody.BULLETLIST:
                if not body.startswith("- "):
                    return LintError(line, f"Body for header '{self.header}' must start with a bullet point")
                return
            
            case InputTypeBody.CHECKLIST:
                pattern = re.compile(r"- \[(x|\s)\]")
                
                if not pattern.match(body):
                    return LintError(line, f"Body for header '{self.header}' must start with a checklist item")
                return
            
            case InputTypeBody.GH_REFERENCE:
                pattern = re.compile(r"#\d+")
                
                if not pattern.match(body):
                    return LintError(line, f"Body for header '{self.header}' must reference a GitHub issue or PR")
                return
            
            case InputTypeBody.ANY:
                return
        
            case _:
                return LintError(line, f"Invalid input type for header '{self.header}'")
            
    def __eq__(self, other):
        if isinstance(other, str):
            return self.header == other
        if isinstance(other, InputType):
            return self.header == other.header
        return NotImplemented
    
    def __str__(self):
        return self.header