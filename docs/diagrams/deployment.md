# Simple DNS Daemon - Deployment Diagrams

## Basic Deployment Architecture

```mermaid
graph TB
    subgraph "Client Network"
        Client1[DNS Client 1]
        Client2[DNS Client 2]
        ClientN[DNS Client N]
    end

    subgraph "DNS Server"
        Server[simple-dnsd<br/>Main Process]
        ZoneFiles[/etc/simple-dnsd/zones/<br/>Zone Files]
        Config[/etc/simple-dnsd/<br/>Configuration]
        Cache[/var/cache/simple-dnsd/<br/>Cache Files]
        Logs[/var/log/simple-dnsd/<br/>Log Files]
    end

    subgraph "System Services"
        Systemd[systemd<br/>Service Manager]
        Logrotate[logrotate<br/>Log Rotation]
    end

    subgraph "Upstream DNS"
        Upstream1[Upstream DNS 1<br/>8.8.8.8]
        Upstream2[Upstream DNS 2<br/>8.8.4.4]
    end

    Client1 --> Server
    Client2 --> Server
    ClientN --> Server

    Systemd --> Server
    Systemd --> Config

    Server --> ZoneFiles
    Server --> Config
    Server --> Cache
    Server --> Logs

    Server --> Upstream1
    Server --> Upstream2

    Logrotate --> Logs
```

## Primary-Secondary DNS Setup

```mermaid
graph TB
    subgraph "Primary DNS Server"
        Primary[simple-dnsd<br/>Primary Server]
        PrimaryZones[Zone Files<br/>Master Zones]
        PrimaryConfig[Configuration<br/>Primary]
    end

    subgraph "Secondary DNS Server"
        Secondary[simple-dnsd<br/>Secondary Server]
        SecondaryZones[Zone Files<br/>Slave Zones]
        SecondaryConfig[Configuration<br/>Secondary]
    end

    subgraph "Clients"
        Client1[Client 1]
        Client2[Client 2]
        ClientN[Client N]
    end

    subgraph "Zone Transfer"
        AXFR[AXFR<br/>Full Transfer]
        IXFR[IXFR<br/>Incremental Transfer]
    end

    Primary --> PrimaryZones
    Primary --> PrimaryConfig

    Secondary --> SecondaryZones
    Secondary --> SecondaryConfig

    Primary -.->|Zone Transfer| AXFR
    Primary -.->|Zone Updates| IXFR
    AXFR --> Secondary
    IXFR --> Secondary

    Client1 --> Primary
    Client1 --> Secondary
    Client2 --> Primary
    Client2 --> Secondary
    ClientN --> Primary
    ClientN --> Secondary
```

## Load Balanced DNS Deployment

```mermaid
graph TB
    subgraph "Load Balancer"
        LB[Load Balancer<br/>DNS Query Distribution]
    end

    subgraph "DNS Server Pool"
        Server1[simple-dnsd<br/>Server 1]
        Server2[simple-dnsd<br/>Server 2]
        Server3[simple-dnsd<br/>Server 3]
        ServerN[simple-dnsd<br/>Server N]
    end

    subgraph "Shared Storage"
        SharedZones[Shared Zone Storage<br/>NFS/GlusterFS]
        SharedCache[Shared Cache<br/>Redis/Memcached]
    end

    subgraph "Clients"
        Client1[Client 1]
        Client2[Client 2]
        ClientN[Client N]
    end

    Client1 --> LB
    Client2 --> LB
    ClientN --> LB

    LB --> Server1
    LB --> Server2
    LB --> Server3
    LB --> ServerN

    Server1 --> SharedZones
    Server1 --> SharedCache
    Server2 --> SharedZones
    Server2 --> SharedCache
    Server3 --> SharedZones
    Server3 --> SharedCache
    ServerN --> SharedZones
    ServerN --> SharedCache
```

## Container Deployment

```mermaid
graph TB
    subgraph "Docker Host"
        Docker[Docker Engine]
        Container[simple-dnsd<br/>Container]
        Volumes[Volumes<br/>Zones/Config/Cache]
        HostNetwork[Host Network<br/>Port 53 UDP/TCP]
    end
    
    subgraph "External"
        Clients[DNS Clients]
        ZoneMount[/host/zones<br/>Zone Files]
        ConfigMount[/host/config<br/>Configuration]
        CacheMount[/host/cache<br/>Cache]
        LogMount[/host/logs<br/>Log Files]
    end
    
    Clients --> HostNetwork
    HostNetwork --> Container
    Container --> Volumes
    
    ZoneMount --> Volumes
    ConfigMount --> Volumes
    CacheMount --> Volumes
    LogMount --> Volumes
    
    Docker --> Container
```

## Kubernetes Deployment

```mermaid
graph TB
    subgraph "Kubernetes Namespace: dns"
        Deployment[Deployment<br/>simple-dnsd]
        Service[Service<br/>LoadBalancer]
        ConfigMap[ConfigMap<br/>Configuration]
        Secret[Secret<br/>TSIG Keys]
        PVC[PersistentVolumeClaim<br/>Zone Files]
    end
    
    subgraph "Kubernetes Nodes"
        Pod1[Pod 1<br/>simple-dnsd]
        Pod2[Pod 2<br/>simple-dnsd]
        PodN[Pod N<br/>simple-dnsd]
    end
    
    subgraph "External"
        Clients[DNS Clients]
        Upstream[Upstream DNS Servers]
    end
    
    Clients --> Service
    Service --> Pod1
    Service --> Pod2
    Service --> PodN
    
    Deployment --> Pod1
    Deployment --> Pod2
    Deployment --> PodN
    
    Pod1 --> ConfigMap
    Pod1 --> Secret
    Pod1 --> PVC
    
    Pod2 --> ConfigMap
    Pod2 --> Secret
    Pod2 --> PVC
    
    PodN --> ConfigMap
    PodN --> Secret
    PodN --> PVC
    
    Pod1 --> Upstream
    Pod2 --> Upstream
    PodN --> Upstream
```

## Split-Horizon DNS

```mermaid
graph TB
    subgraph "DNS Server"
        Server[simple-dnsd<br/>Split-Horizon Server]
        InternalView[Internal View<br/>Private Zones]
        ExternalView[External View<br/>Public Zones]
    end

    subgraph "Internal Network"
        InternalClient1[Internal Client 1<br/>192.168.1.0/24]
        InternalClient2[Internal Client 2<br/>192.168.1.0/24]
    end

    subgraph "External Network"
        ExternalClient1[External Client 1<br/>Internet]
        ExternalClient2[External Client 2<br/>Internet]
    end

    InternalClient1 -->|Internal Query| Server
    InternalClient2 -->|Internal Query| Server
    ExternalClient1 -->|External Query| Server
    ExternalClient2 -->|External Query| Server

    Server -->|Internal Queries| InternalView
    Server -->|External Queries| ExternalView
```
