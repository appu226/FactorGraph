import logging

###### utility enum for parsing log verbosity ######
LogLevelMapping: dict[str, int] = {
    'QUIET': logging.CRITICAL,
    'ERROR': logging.ERROR,
    'WARNING': logging.WARN,
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG
}