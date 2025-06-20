// Code generated by protoc-gen-go-grpc. DO NOT EDIT.
// versions:
// - protoc-gen-go-grpc v1.5.1
// - protoc             v3.21.12
// source: tweet.proto

package proto

import (
	context "context"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
)

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
// Requires gRPC-Go v1.64.0 or later.
const _ = grpc.SupportPackageIsVersion9

const (
	TweetService_SendTweet_FullMethodName = "/tweet.TweetService/SendTweet"
)

// TweetServiceClient is the client API for TweetService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://pkg.go.dev/google.golang.org/grpc/?tab=doc#ClientConn.NewStream.
type TweetServiceClient interface {
	SendTweet(ctx context.Context, in *Tweet, opts ...grpc.CallOption) (*Response, error)
}

type tweetServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewTweetServiceClient(cc grpc.ClientConnInterface) TweetServiceClient {
	return &tweetServiceClient{cc}
}

func (c *tweetServiceClient) SendTweet(ctx context.Context, in *Tweet, opts ...grpc.CallOption) (*Response, error) {
	cOpts := append([]grpc.CallOption{grpc.StaticMethod()}, opts...)
	out := new(Response)
	err := c.cc.Invoke(ctx, TweetService_SendTweet_FullMethodName, in, out, cOpts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// TweetServiceServer is the server API for TweetService service.
// All implementations must embed UnimplementedTweetServiceServer
// for forward compatibility.
type TweetServiceServer interface {
	SendTweet(context.Context, *Tweet) (*Response, error)
	mustEmbedUnimplementedTweetServiceServer()
}

// UnimplementedTweetServiceServer must be embedded to have
// forward compatible implementations.
//
// NOTE: this should be embedded by value instead of pointer to avoid a nil
// pointer dereference when methods are called.
type UnimplementedTweetServiceServer struct{}

func (UnimplementedTweetServiceServer) SendTweet(context.Context, *Tweet) (*Response, error) {
	return nil, status.Errorf(codes.Unimplemented, "method SendTweet not implemented")
}
func (UnimplementedTweetServiceServer) mustEmbedUnimplementedTweetServiceServer() {}
func (UnimplementedTweetServiceServer) testEmbeddedByValue()                      {}

// UnsafeTweetServiceServer may be embedded to opt out of forward compatibility for this service.
// Use of this interface is not recommended, as added methods to TweetServiceServer will
// result in compilation errors.
type UnsafeTweetServiceServer interface {
	mustEmbedUnimplementedTweetServiceServer()
}

func RegisterTweetServiceServer(s grpc.ServiceRegistrar, srv TweetServiceServer) {
	// If the following call pancis, it indicates UnimplementedTweetServiceServer was
	// embedded by pointer and is nil.  This will cause panics if an
	// unimplemented method is ever invoked, so we test this at initialization
	// time to prevent it from happening at runtime later due to I/O.
	if t, ok := srv.(interface{ testEmbeddedByValue() }); ok {
		t.testEmbeddedByValue()
	}
	s.RegisterService(&TweetService_ServiceDesc, srv)
}

func _TweetService_SendTweet_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(Tweet)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TweetServiceServer).SendTweet(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: TweetService_SendTweet_FullMethodName,
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TweetServiceServer).SendTweet(ctx, req.(*Tweet))
	}
	return interceptor(ctx, in, info, handler)
}

// TweetService_ServiceDesc is the grpc.ServiceDesc for TweetService service.
// It's only intended for direct use with grpc.RegisterService,
// and not to be introspected or modified (even as a copy)
var TweetService_ServiceDesc = grpc.ServiceDesc{
	ServiceName: "tweet.TweetService",
	HandlerType: (*TweetServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "SendTweet",
			Handler:    _TweetService_SendTweet_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "tweet.proto",
}
