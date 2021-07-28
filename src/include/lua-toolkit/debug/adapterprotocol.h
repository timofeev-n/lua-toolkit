//◦ Playrix ◦
// Debug Adapter Protocol spec: https://microsoft.github.io/debug-adapter-protocol/specification
// https://microsoft.github.io/debug-adapter-protocol/
#pragma once
#include <runtime/serialization/runtimevalue.h>
#include <runtime/serialization/serialization.h>
#include <runtime/meta/classinfo.h>
//#include <boost/optional.hpp>
#include <optional>
#include <limits>

namespace Runtime::Dap {

/**
	Base class of requests, responses, and events.
*/
struct ProtocolMessage
{
	static constexpr unsigned InvalidSeq = std::numeric_limits<unsigned>::max();

	static constexpr std::string_view MessageRequest {"request"};
	static constexpr std::string_view MessageEvent {"event"};
	static constexpr std::string_view MessageResponse {"response"};


#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(seq, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(type, Serialization::RequiredFieldAttribute{})
		)
	)
#pragma endregion

	/**
		Sequence number (also known as message ID). For protocol messages of type 'request' this ID can be used to cancel the request.
	*/
	unsigned seq = InvalidSeq;

	/**
		Message type.
		Values: 'request', 'response', 'event', etc.
	*/
	std::string type;

	ProtocolMessage() = default;

	ProtocolMessage(unsigned messageSeq, std::string messageType);
};

/**
	A client or debug adapter initiated request.
*/
struct RequestMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_FIELD(command, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(arguments)
		)
	)
#pragma endregion

	/**
		The command to execute.
	*/
	std::string command;

	/**
		Object containing arguments for the command.
	*/
	RuntimeValue::Ptr arguments;
};

/**
	A debug adapter initiated event.
*/
struct EventMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_NAMED_FIELD(eventType, "event", Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(body)
		)
	)
#pragma endregion

	/**
		Type of event.
	*/
	std::string eventType;

	/**
		Event-specific information.
	*/
	RuntimeValue::Ptr body;
};

/**
	Response for a request.
*/
struct ResponseMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_FIELD(request_seq, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(success, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(command),
			CLASS_FIELD(message)
		)
	)
#pragma endregion

	/**
		Sequence number of the corresponding request.
	*/
	unsigned request_seq = ProtocolMessage::InvalidSeq;

	/**
		Outcome of the request.
			If true, the request was successful and the 'body' attribute may contain the result of the request.
			If the value is false, the attribute 'message' contains the error in short form and the 'body' may contain additional information (see 'ErrorResponse.body.error').
	*/
	bool success = true;

	/**
		The command requested.
	*/
	std::string command;

	/**
		Contains the raw error in short form if 'success' is false.
		This raw error might be interpreted by the frontend and is not shown in the UI.
		Some predefined values exist.
		Values:
			'cancelled': request was cancelled.
			etc.
	*/
	std::optional<std::string> message;


	ResponseMessage () = default;

	ResponseMessage(unsigned messageSeq, const RequestMessage& request);

	ResponseMessage& SetError(std::string_view errorMessage);
};

struct GenericResponse : ResponseMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ResponseMessage),

		CLASS_FIELDS(
			CLASS_FIELD(body)
		)
	)
#pragma endregion

	using ResponseMessage::ResponseMessage;

	/**
		Contains request result if success is true and optional error details if success is false.
	*/
	RuntimeValue::Ptr body;
};


/**
	The checksum of an item calculated by the specified algorithm.
*/
struct Checksum {

#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(algorithm, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(checksum, Serialization::RequiredFieldAttribute{})
		)
	)
#pragma endregion

	/**
		The algorithm used to calculate this checksum.
		'MD5' | 'SHA1' | 'SHA256' | 'timestamp';
		*/
	std::string algorithm;

	/**
		Value of the checksum.
	*/
	std::string checksum;
};

/**
	A Source is a descriptor for source code.
	It is returned from the debug adapter as part of a StackFrame and it is used by clients when specifying breakpoints.
*/

struct Source {

#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(path),
			CLASS_FIELD(sourceReference),
			CLASS_FIELD(presentationHint),
			CLASS_FIELD(origin),
			CLASS_FIELD(sources),
			CLASS_FIELD(adapterData),
			CLASS_FIELD(checksums)
		)
	)
#pragma endregion

	/**
		The short name of the source. Every source returned from the debug adapter has a name.
		When sending a source to the debug adapter this name is optional.
	*/
	std::string name;

	/**
		The path of the source to be shown in the UI.
		It is only used to locate and load the content of the source if no sourceReference is specified (or its value is 0).
	*/
	std::string path;

	/**
		If sourceReference > 0 the contents of the source must be retrieved through the SourceRequest (even if a path is specified).
		A sourceReference is only valid for a session, so it must not be used to persist a source.
		The value should be less than or equal to 2147483647 (2^31-1).
	*/
	std::optional<int> sourceReference;

	/**
		An optional hint for how to present the source in the UI.
		A value of 'deemphasize' can be used to indicate that the source is not available or that it is skipped on stepping.
	*/
	std::string presentationHint; // 'normal' | 'emphasize' | 'deemphasize'

	/**
		The (optional) origin of this source: possible values 'internal module', 'inlined content from source map', etc.
	*/
	std::string origin;

	/**
		An optional list of sources that are related to this source. These may be the source that generated this source.
	*/
	std::vector<Source> sources;

	/**
		Optional data that a debug adapter might want to loop through the client.
		The client should leave the data intact and persist it across sessions. The client should not interpret the data.
	*/
	RuntimeValue::Ptr adapterData;

	/**
		The checksums associated with this file.
	*/
	std::vector<Checksum>checksums;
};

/**
	Information about a Breakpoint created in setBreakpoints, setFunctionBreakpoints, setInstructionBreakpoints, or setDataBreakpoints.
*/
struct Breakpoint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(id),
			CLASS_FIELD(verified),
			CLASS_FIELD(message),
			CLASS_FIELD(source),
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(endLine),
			CLASS_FIELD(endColumn),
			CLASS_FIELD(instructionReference),
			CLASS_FIELD(offset)
		)
	)
#pragma endregion

	/* An optional identifier for the breakpoint. It is needed if breakpoint events are used to update or remove breakpoints. */
	std::optional<int> id;

	/* If true breakpoint could be set (but not necessarily at the desired location). */
	bool verified;

	/**
		An optional message about the state of the breakpoint.
		This is shown to the user and can be used to explain why a breakpoint could not be verified.
	*/
	std::string message;
	
	/* The source where the breakpoint is located. */
	std::optional<Source> source;

	/* The start line of the actual range covered by the breakpoint. */
	std::optional<unsigned> line;

	/* An optional start column of the actual range covered by the breakpoint. */
	std::optional<unsigned> column;

	/* An optional end line of the actual range covered by the breakpoint. */
	std::optional<unsigned>  endLine;
	
	/** An optional end column of the actual range covered by the breakpoint.
			If no end line is given, then the end column is assumed to be in the start line.
	*/
	std::optional<unsigned> endColumn;

	/* An optional memory reference to where the breakpoint is set. */
	std::string instructionReference;

	/**
		An optional offset from the instruction reference.
		This can be negative.
	*/
	std::optional<unsigned> offset;


	Breakpoint() = default;
};


/**
	Properties of a breakpoint or logpoint passed to the setBreakpoints request.
*/
struct SourceBreakpoint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(condition),
			CLASS_FIELD(hitCondition),
			CLASS_FIELD(logMessage)
		)
	)
#pragma endregion

	/**
		The source line of the breakpoint or logpoint.
	*/
	unsigned line;

	/**
		An optional source column of the breakpoint.
	*/
	std::optional<unsigned> column;

	/** An optional expression for conditional breakpoints.
			It is only honored by a debug adapter if the capability 'supportsConditionalBreakpoints' is true.
	*/
	std::string condition;

	/** An optional expression that controls how many hits of the breakpoint are ignored.
			The backend is expected to interpret the expression as needed.
			The attribute is only honored by a debug adapter if the capability 'supportsHitConditionalBreakpoints' is true.
	*/
	std::string hitCondition;

	/** If this attribute exists and is non-empty, the backend must not 'break' (stop)
			but log the message instead. Expressions within {} are interpolated.
			The attribute is only honored by a debug adapter if the capability 'supportsLogPoints' is true.
	*/
	std::string logMessage;
};




/**
	Arguments for 'setBreakpoints' request.
*/
struct SetBreakpointsArguments
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(source),
			CLASS_FIELD(breakpoints),
			CLASS_FIELD(sourceModified)
		)
	)
#pragma endregion

	/**
		The source location of the breakpoints; either 'source.path' or 'source.reference' must be specified.
	*/
	Source source;

	/**
		The code locations of the breakpoints.
	*/
	std::vector<SourceBreakpoint> breakpoints;
	
	/**
		A value of true indicates that the underlying source has been modified which results in new breakpoint locations.
	*/
	std::optional<bool> sourceModified;
};

/** Response to 'setBreakpoints' request.
		Returned is information about each breakpoint created by this request.
		This includes the actual code location and whether the breakpoint could be verified.
		The breakpoints returned are in the same order as the elements of the 'breakpoints'
		(or the deprecated 'lines') array in the arguments.
*/
struct SetBreakpointsResponseBody
{
#pragma region Class info
CLASS_INFO(
	CLASS_FIELDS(
		CLASS_FIELD(breakpoints)
	)
)
#pragma endregion
	/**
		Information about the breakpoints.
		The array elements are in the same order as the elements of the 'breakpoints' (or the deprecated 'lines') array in the arguments.
	*/
	std::vector<Breakpoint> breakpoints;
};


struct SetBreakpointsResponse : ResponseMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ResponseMessage),

		CLASS_FIELDS(
			CLASS_FIELD(body)
		)
	)
#pragma endregion
	 // using ResponseMessage::ResponseMessage;
	SetBreakpointsResponseBody body;
};

} // namespace Runtime::Dap
