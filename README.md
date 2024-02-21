# CADventory

CADventory is a 3D CAD file, categorical tagging, and associated metadata inventory management system.

## Test and Deploy

- [ ] Deploy to a .zip distribution...

## SETUP TODO

- The project shall demonstrate CI testing for at least one platform.
- The project shall incorporate some code coverage testing.
- The project shall incorporate some code quality testing.
- The project shall set up backups of all project data.


## Installation

TODO: Document installation steps


## Usage

TODO: Document basic usage


## Support

TODO: Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.


## Roadmap

TODO: Add from project proposal.


## Design

Here's an architecture diagram:

```
┌──────────────────────────────────────────────────────────────────────┐
│                               CADventory                             │
│                                                                      │
│    ┌──────────────────────────────────────────────────────────────┐  │
│    │                      User Interface (Qt)                     │  │
│    └─────────────┬───────────────────────┬─────────────────┬──────┘  │
│                  │                       │                 │         │
│    ┌─────────────▼───────────┐  ┌────────▼─────────┐  ┌────▼──────┐  │
│    │  Filesystem Processor   │  │ SQLite or JSON   │◄─│ Report    │  │
│    └─────────────┬───────────┘  │ Storage Manager  │  │ Generator │  │
│                  │              └──────────────────┘  └────┬──────┘  │
│                  │                                         │         │
│    ┌─────────────▼───────────┐                             │         │
│    │ Geometry/Image/Document │                             │         │
│    │ Handler                 │                             │         │
│    └─────────────┬───────────┘                             │         │
│                  │                                         │         │
│    ┌─────────────▼────────┐                                │         │
│    │     CAD Libraries    │◄───────────────────────────────┘         │
│    └──────────────────────┘                                          │
└──────────────────────────────────────────────────────────────────────┘
```


## Contributing

TODO: Document the testing/linting steps.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.


## Authors and acknowledgment

TODO: Show your appreciation to those who have contributed to the project.

## License

MIT

