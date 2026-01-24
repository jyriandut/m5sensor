from pydantic import BaseModel
from datetime import datetime

class Device(BaseModel):
    id: str
    first_seen: datetime
