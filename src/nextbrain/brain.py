"""
NeXTBrain

Core application object.

This module contains the main entry point for the NeXTBrain
application. Additional services will be added over time while
keeping the public interface simple and stable.
"""


class NeXTBrain:
    """Main NeXTBrain application."""

    def __init__(self):
        """Initialize a NeXTBrain instance."""
        self.version = "0.0.1"
    def __str__(self):
        return f"NeXTBrain v{self.version}"
