/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_SERVER_H
#define YOJIMBO_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_adapter.h"
#include "yojimbo_allocator.h"
#include "yojimbo_connection.h"

/** @file */

struct netcode_server_t;
struct reliable_endpoint_t;

namespace yojimbo
{
    class Connection;
    class NetworkSimulator;

    /**
        Server interface
     */

    class ServerInterface
    {
    public:

        virtual ~ServerInterface() {}

        /**
            Set the context for reading and writing packets.

            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.

            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 

            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Start the server and allocate client slots.
            
            Each client that connects to this server occupies one of the client slots allocated by this function.

            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]

            @see Server::Stop
         */

        virtual void Start( int maxClients ) = 0;

        /**
            Stop the server and free client slots.
        
            Any clients that are connected at the time you call stop will be disconnected.
            
            When the server is stopped, clients cannot connect to the server.

            @see Server::Start.
         */

        virtual void Stop() = 0;

        /**
            Disconnect the client at the specified client index.
            
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.

            @see Server::IsClientConnected
         */

        virtual void DisconnectClient( int clientIndex ) = 0;

        /**
            Disconnect all clients from the server.
            
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
         */

        virtual void DisconnectAllClients() = 0;

        /**
            Send packets to connected clients.
         
            This function drives the sending of packets that transmit messages to clients.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from connected clients.

            This function drives the procesing of messages included in packets received from connected clients.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance server time.

            Call this at the end of each frame to advance the server time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the server running?

            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.

            Clients can only connect to the server while it is running.

            @returns true if the server is currently running.
         */

        virtual bool IsRunning() const = 0;

        /**
            Get the maximum number of clients that can connect to the server.

            Corresponds to the maxClients parameter passed into the last call to Server::Start.

            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        virtual int GetMaxClients() const = 0;

        /**
            Is a client connected to a client slot?

            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.

            @returns True if the client is connected.
         */

        virtual bool IsClientConnected( int clientIndex ) const = 0;

        /** 
            Get the number of clients that are currently connected to the server.

            @returns the number of connected clients.
         */

        virtual int GetNumConnectedClients() const = 0;

        /**
            Gets the current server time.

            @see Server::AdvanceTime
         */

        virtual double GetTime() const = 0;

        // todo: document these methods

        virtual Message * CreateMessage( int clientIndex, int type ) = 0;

        virtual uint8_t * AllocateBlock( int clientIndex, int bytes ) = 0;

        virtual void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) = 0;

        virtual void FreeBlock( int clientIndex, uint8_t * block ) = 0;

        virtual bool CanSendMessage( int clientIndex, int channelIndex ) const = 0;

        virtual void SendMessage( int clientIndex, int channelIndex, Message * message ) = 0;

        virtual Message * ReceiveMessage( int clientIndex, int channelIndex ) = 0;

        virtual void ReleaseMessage( int clientIndex, Message * message ) = 0;
    };

    /**
        Functionality common across all server implementations.
     */

    class BaseServer : public ServerInterface
    {
    public:

        BaseServer( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time );

        ~BaseServer();

        void SetContext( void * context );

        void Start( int maxClients );

        void Stop();

        void AdvanceTime( double time );

        bool IsRunning() const { return m_running; }

        int GetMaxClients() const { return m_maxClients; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int clientIndex, int type );

        uint8_t * AllocateBlock( int clientIndex, int bytes );

        void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes );

        void FreeBlock( int clientIndex, uint8_t * block );

        bool CanSendMessage( int clientIndex, int channelIndex ) const;

        void SendMessage( int clientIndex, int channelIndex, Message * message );

        Message * ReceiveMessage( int clientIndex, int channelIndex );

        void ReleaseMessage( int clientIndex, Message * message );

    protected:

        void * GetContext() { return m_context; }

        Allocator & GetGlobalAllocator() { yojimbo_assert( m_globalAllocator ); return *m_globalAllocator; }

        MessageFactory & GetClientMessageFactory( int clientIndex );

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetClientEndpoint( int clientIndex );

        Connection & GetClientConnection( int clientIndex );

        virtual void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );
        
        static int StaticProcessPacketFunction( void * context,int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );
        
        static void StaticFreeFunction( void * context, void * pointer );

    private:

        BaseClientServerConfig m_config;                            ///< Base client/server config.
        Allocator * m_allocator;                                    ///< Allocator passed in to constructor.
        Adapter * m_adapter;                                        ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                           ///< Optional serialization context.
        int m_maxClients;                                           ///< Maximum number of clients supported.
        bool m_running;                                             ///< True if server is currently running, eg. after "Start" is called, before "Stop".
        double m_time;                                              ///< Current server time in seconds.
        uint8_t * m_globalMemory;                                   ///< The block of memory backing the global allocator. Allocated with m_allocator.
        uint8_t * m_clientMemory[MaxClients];                       ///< The block of memory backing the per-client allocators. Allocated with m_allocator.
        Allocator * m_globalAllocator;                              ///< The global allocator. Used for allocations that don't belong to a specific client.
        Allocator * m_clientAllocator[MaxClients];                  ///< Array of per-client allocator. These are used for allocations related to connected clients.
        MessageFactory * m_clientMessageFactory[MaxClients];        ///< Array of per-client message factories. This silos message allocations per-client slot.
        Connection * m_clientConnection[MaxClients];                ///< Array of per-client connection classes. This is how messages are exchanged with clients.
        reliable_endpoint_t * m_clientEndpoint[MaxClients];         ///< Array of per-client reliable.io endpoints.
        NetworkSimulator * m_networkSimulator;                      ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional. 
    };

    /**
        Dedicated server implementation.
     */

    class Server : public BaseServer
    {
    public:

        Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Server();

        void Start( int maxClients );

        void Stop();

        void DisconnectClient( int clientIndex );

        void DisconnectAllClients();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsClientConnected( int clientIndex ) const;

        int GetNumConnectedClients() const;

    private:

        void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void ConnectDisconnectCallbackFunction( int clientIndex, int connected );

        static void StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected );

        ClientServerConfig m_config;
        netcode_server_t * m_server;
        Address m_address;
        uint8_t m_privateKey[KeyBytes];
    };
}

#endif // #ifndef YOJIMBO_SERVER_H
